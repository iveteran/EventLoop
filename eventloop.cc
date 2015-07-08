#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <set>
#include <map>

#include "eventloop.h"

#define MAX_BYTES_RECEIVE       1024

using std::set;
using std::map;

namespace eventloop {

// singleton class that manages all signals
class SignalManager {
 public:
  int AddEvent(BaseSignalEvent *e);
  int DeleteEvent(BaseSignalEvent *e);
  int UpdateEvent(BaseSignalEvent *e);

 private:
  friend void SignalHandler(int signo);
  map<int, set<BaseSignalEvent *> > sig_events_;

 public:
  static SignalManager *Instance() {
    if (!instance_) {
      instance_ = new SignalManager();
    }
    return instance_;
  }
 private:
  SignalManager() {}
 private:
  static SignalManager *instance_;
};

SignalManager *SignalManager::instance_ = NULL;

class TimerManager {
 public:
  int AddEvent(BaseTimerEvent *e);
  int DeleteEvent(BaseTimerEvent *e);
  int UpdateEvent(BaseTimerEvent *e);

 private:
  friend class EventLoop;
  class Compare {
   public:
    bool operator()(const BaseTimerEvent *e1, const BaseTimerEvent *e2) {
      timeval t1 = e1->Time();
      timeval t2 = e2->Time();
      return (t1.tv_sec < t2.tv_sec) || (t1.tv_sec == t2.tv_sec && t1.tv_usec < t2.tv_usec);
    }
  };

  typedef set<BaseTimerEvent *, Compare> TimerSet;

 private:
  TimerSet timers_;
};

int SetNonblocking(int fd) {
  int opts;
  if ((opts = fcntl(fd, F_GETFL)) != -1) {
    opts = opts | O_NONBLOCK;
    if(fcntl(fd, F_SETFL, opts) != -1) {
      return 0;
    }
  }
  return -1;
}

int ConnectTo(const char *host, short port, bool async) {
  int fd;
  sockaddr_in addr;

  if (host == NULL) return -1;

  fd = socket(AF_INET, SOCK_STREAM, 0);
  addr.sin_family = PF_INET;
  addr.sin_port = htons(port);
  if (host[0] == '\0' || strcmp(host, "localhost") == 0) {
    inet_aton("127.0.0.1", &addr.sin_addr);
  } else if (!strcmp(host, "any")) {
    addr.sin_addr.s_addr = INADDR_ANY;
  } else {
    inet_aton(host, &addr.sin_addr);
  }

  if (async) SetNonblocking(fd);

  if (connect(fd, (sockaddr*)&addr, sizeof(sockaddr_in)) == -1) {
    if (errno != EINPROGRESS) {
      close(fd);
      return -1;
    }
  }

  return fd;
}

int BindTo(const char *host, short port) {
  int fd, on = 1;
  sockaddr_in addr;

  if (host == NULL) return -1;

  if ((fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    return -1;
  }

  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

  memset(&addr, 0, sizeof(sockaddr_in));
  addr.sin_family = PF_INET;
  addr.sin_port = htons(port);
  if (host[0] == '\0' || strcmp(host, "localhost") == 0) {
    inet_aton("127.0.0.1", &addr.sin_addr);
  } else if (strcmp(host, "any") == 0) {
    addr.sin_addr.s_addr = INADDR_ANY;
  } else {
    if (inet_aton(host, &addr.sin_addr) == 0) return -1;
  }

  if (bind(fd, (sockaddr*)&addr, sizeof(sockaddr_in)) == -1 || listen(fd, 10) == -1) {
    return -1;
  }

  return fd;
}

// return time diff in ms
static int TimeDiff(timeval tv1, timeval tv2) {
  return (tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000;
}

// add time in ms to tv
static timeval TimeAdd(timeval tv1, timeval tv2) {
  timeval t = tv1;
  t.tv_sec += tv2.tv_sec;
  t.tv_usec += tv2.tv_usec;
  t.tv_sec += t.tv_usec / 1000000;
  t.tv_usec %= 1000000;
  return t;
}

// TimerManager implementation
int TimerManager::AddEvent(BaseTimerEvent *e) {
  return !timers_.insert(e).second;
}

int TimerManager::DeleteEvent(BaseTimerEvent *e) {
  return timers_.erase(e) != 1;
}

int TimerManager::UpdateEvent(BaseTimerEvent *e) {
  timers_.erase(e);
  return !timers_.insert(e).second;
}

// PeriodicTimerEvent implementation
void PeriodicTimerEvent::OnEvents(uint32_t events) {
  OnTimer();
  time_ = TimeAdd(el_->Now(), interval_);
  el_->UpdateEvent(this);
}

void PeriodicTimerEvent::Start() {
  if (!el_) return;
  running_ = true;
  timeval tv;
  gettimeofday(&tv, NULL);
  SetTime(tv);
  el_->AddEvent(this);
}

void PeriodicTimerEvent::Stop() {
  if (!el_) return;
  running_ = false;
  el_->DeleteEvent(this);
}

// EventLoop implementation
EventLoop::EventLoop() {
  epfd_ = epoll_create(256);
  timermanager_ = new TimerManager();
}

EventLoop::~EventLoop() {
  close(epfd_);
  static_cast<TimerManager *>(timermanager_)->timers_.clear();
  delete static_cast<TimerManager *>(timermanager_);
}

int EventLoop::CollectFileEvents(int timeout) {
  return epoll_wait(epfd_, evs_, 256, timeout);
}

int EventLoop::DoTimeout() {
  int n = 0;
  TimerManager::TimerSet& timers = static_cast<TimerManager *>(timermanager_)->timers_;
  TimerManager::TimerSet::iterator ite = timers.begin();
  while (ite != timers.end()) {
    timeval tv = (*ite)->Time();
    if (TimeDiff(now_, tv) < 0) break;
    n++;
    BaseTimerEvent *e = *ite;
    timers.erase(ite);
    e->OnEvents(BaseTimerEvent::TIMER);
    ite = timers.begin();
  }
  return n;
}

int EventLoop::ProcessEvents(int timeout) {
  int i, nt, n;

  n = CollectFileEvents(timeout);

  gettimeofday(&now_, NULL);

  nt = DoTimeout();

  for(i = 0; i < n; i++) {
    BaseEvent *e = (BaseEvent *)evs_[i].data.ptr;
    uint32_t events = 0;
    if (evs_[i].events & EPOLLIN) events |= BaseFileEvent::READ;
    if (evs_[i].events & EPOLLOUT) events |= BaseFileEvent::WRITE;
    if (evs_[i].events & (EPOLLHUP | EPOLLERR)) events |= BaseFileEvent::ERROR;
    e->OnEvents(events);
  }

  return nt + n;
}

void EventLoop::StopLoop() {
  stop_ = true;
}

void EventLoop::StartLoop() {
  stop_ = false;
  while (!stop_) {
    int timeout = 100;
    gettimeofday(&now_, NULL);

    if (static_cast<TimerManager *>(timermanager_)->timers_.size() > 0) {
      TimerManager::TimerSet::iterator ite = static_cast<TimerManager *>(timermanager_)->timers_.begin();
      timeval tv = (*ite)->Time();
      int t = TimeDiff(tv, now_);
      if (t > 0 && timeout > t) timeout = t;
    }

    ProcessEvents(timeout);
  }
}

int EventLoop::AddEvent(BaseFileEvent *e) {
  epoll_event ev = {0, {0}};
  uint32_t events = e->events_;

  ev.events = 0;
  if (events & BaseFileEvent::READ) ev.events |= EPOLLIN;
  if (events & BaseFileEvent::WRITE) ev.events |= EPOLLOUT;
  if (events & BaseFileEvent::ERROR) ev.events |= EPOLLHUP | EPOLLERR;
  ev.data.fd = e->file;
  ev.data.ptr = e;

  SetNonblocking(e->file);

  return epoll_ctl(epfd_, EPOLL_CTL_ADD, e->file, &ev);
}

int EventLoop::UpdateEvent(BaseFileEvent *e) {
  epoll_event ev = {0, {0}};
  uint32_t events = e->events_;

  ev.events = 0;
  if (events & BaseFileEvent::READ) ev.events |= EPOLLIN;
  if (events & BaseFileEvent::WRITE) ev.events |= EPOLLOUT;
  if (events & BaseFileEvent::ERROR) ev.events |= EPOLLHUP | EPOLLERR;
  ev.data.fd = e->file;
  ev.data.ptr = e;

  return epoll_ctl(epfd_, EPOLL_CTL_MOD, e->file, &ev);
}

int EventLoop::DeleteEvent(BaseFileEvent *e) {
  epoll_event ev; // kernel before 2.6.9 requires
  return epoll_ctl(epfd_, EPOLL_CTL_DEL, e->file, &ev);
}

int EventLoop::AddEvent(BaseTimerEvent *e) {
  return static_cast<TimerManager *>(timermanager_)->AddEvent(e);
}

int EventLoop::UpdateEvent(BaseTimerEvent *e) {
  return static_cast<TimerManager *>(timermanager_)->UpdateEvent(e);
}

int EventLoop::DeleteEvent(BaseTimerEvent *e) {
  return static_cast<TimerManager *>(timermanager_)->DeleteEvent(e);
}

int EventLoop::AddEvent(BaseSignalEvent *e) {
  return SignalManager::Instance()->AddEvent(e);
}

int EventLoop::DeleteEvent(BaseSignalEvent *e) {
  return SignalManager::Instance()->DeleteEvent(e);
}

int EventLoop::UpdateEvent(BaseSignalEvent *e) {
  return SignalManager::Instance()->UpdateEvent(e);
}

int EventLoop::AddEvent(BufferFileEvent *e) {
  AddEvent(dynamic_cast<BaseFileEvent *>(e));
  e->el_ = this;
  return 0;
}

int EventLoop::AddEvent(PeriodicTimerEvent *e) {
  AddEvent(dynamic_cast<BaseTimerEvent *>(e));
  e->el_ = this;
  return 0;
}

void SignalHandler(int signo) {
  set<BaseSignalEvent *> events = SignalManager::Instance()->sig_events_[signo];
  for (set<BaseSignalEvent *>::iterator ite = events.begin(); ite != events.end(); ++ite) {
    (*ite)->OnEvents(signo);
  }
}

int SignalManager::AddEvent(BaseSignalEvent *e) {
  struct sigaction action;
  action.sa_handler = SignalHandler;
  action.sa_flags = SA_RESTART;
  sigemptyset(&action.sa_mask);

  sig_events_[e->Signal()].insert(e);
  sigaction(e->Signal(), &action, NULL);

  return 0;
}

int SignalManager::DeleteEvent(BaseSignalEvent *e) {
  sig_events_[e->Signal()].erase(e);
  return 0;
}

int SignalManager::UpdateEvent(BaseSignalEvent *e) {
  sig_events_[e->Signal()].erase(e);
  sig_events_[e->Signal()].insert(e);
  return 0;
}

// BufferFileEvent implementation
int BufferFileEvent::ReceiveData(string& rtn_data) {
  char buffer[MAX_BYTES_RECEIVE] = {0};
  int len = read(file, buffer, sizeof(buffer));
  if (len <= 0) {
    return len;    // occurs error or received EOF
  }

  recvbuf_ += string(buffer, len);
  if (recvbuf_.size() >= torecv_) {
    if (torecv_ > 0) {
      rtn_data = recvbuf_.substr(0, torecv_);
      recvbuf_ = recvbuf_.substr(torecv_);
    }
    else {
      rtn_data = recvbuf_;
      recvbuf_.clear();
    }
  }
  return (!rtn_data.empty() ? rtn_data.size() : recvbuf_.size());
}

int BufferFileEvent::SendData() {
  uint32_t total_sent = 0;
  while (!sendbuf_list_.empty()) {
    const string& sendbuf = sendbuf_list_.front();
    uint32_t tosend = sendbuf.size();
    int len = write(file, sendbuf.data() + sent_, tosend - sent_);
    if (len < 0) {
      return len;   // occurs error
    }

    sent_ += len;
    total_sent += sent_;
    if (sent_ == tosend) {
      SetEvents(events_ & (~BaseFileEvent::WRITE));
      el_->UpdateEvent(this);
      OnSent(sendbuf);

      sendbuf_list_.pop_front();
      sent_ = 0;
    }
    else {
      break;
    }
  }
  return total_sent;
}

void BufferFileEvent::OnEvents(uint32_t events) {
  if (events & BaseFileEvent::READ) {
    string rtn_data;
    int len = ReceiveData(rtn_data);
    if (len < 0) {
      OnError(strerror(errno));
      return;
    }
    else if (len == 0 ) {
      OnClosed();
      return;
    }

    if (!rtn_data.empty()) {
      OnReceived(rtn_data);
    }
  }

  if (events & BaseFileEvent::WRITE) {
    while (!sendbuf_list_.empty()) {
      int len = SendData();
      if (len < 0) {
        OnError(strerror(errno));
        return;
      }
    }
  }

  if (events & BaseFileEvent::ERROR) {
    OnError(strerror(errno));
    return;
  }

}

void BufferFileEvent::SetReceiveLen(uint32_t len) {
  torecv_ = len;
}

void BufferFileEvent::Send(const char *buffer, uint32_t len) {
  const string sendbuf(buffer, len);
  Send(sendbuf);
}

void BufferFileEvent::Send(const string& buffer) {
  sendbuf_list_.push_back(buffer);
  if (!(events_ & BaseFileEvent::WRITE)) {
    SetEvents(events_ | BaseFileEvent::WRITE);
    el_->UpdateEvent(this);
  }
}

}

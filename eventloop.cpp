#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <set>
#include <map>

#include "eventloop.h"

#define MAX_BYTES_RECEIVE       4096

namespace evt_loop {

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

// singleton class that manages all signals
class SignalManager {
 public:
  int AddEvent(SignalEvent *e);
  int DeleteEvent(SignalEvent *e);
  int UpdateEvent(SignalEvent *e);

 private:
  friend void SignalHandler(int signo);
  std::map<int, std::set<SignalEvent *> > sig_events_;

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
  static SignalManager* instance_;
};

SignalManager *SignalManager::instance_ = NULL;

class TimerManager {
 public:
  int AddEvent(TimerEvent *e);
  int DeleteEvent(TimerEvent *e);
  int UpdateEvent(TimerEvent *e);

 private:
  friend class EventLoop;

  typedef std::set<TimerEvent*> TimerSet;
  typedef std::map<TimeVal, TimerSet> TimerMap;

 private:
  TimerMap timers_;
};

// TimerManager implementation
int TimerManager::AddEvent(TimerEvent *e) {
  //printf("[TimerManager::AddEvent] event object: %p, timeval: (%ld.%ld)\n", e, e->Time().tv_sec, e->Time().tv_usec);
  TimerMap::iterator iter = timers_.find(e->Time());
  if (iter != timers_.end()) {
      iter->second.insert(e);
  } else {
      TimerSet event_set;
      event_set.insert(e);
      timers_.insert(make_pair(e->Time(), event_set));
  }
  return 0;
}

int TimerManager::DeleteEvent(TimerEvent *e) {
  TimerMap::iterator iter = timers_.find(e->Time());
  if (iter != timers_.end()) {
      iter->second.erase(e);
  }
  return 0;
}

int TimerManager::UpdateEvent(TimerEvent *e) {
  TimerMap::iterator iter = timers_.find(e->Time());
  if (iter != timers_.end()) {
      iter->second.erase(e);
      iter->second.insert(e);
  } else {
      TimerSet event_set;
      event_set.insert(e);
      timers_.insert(make_pair(e->Time(), event_set));
  }
  return 0;
}

// PeriodicTimerEvent implementation
void PeriodicTimerEvent::OnEvents(uint32_t events) {
  OnTimer();
  if (running_) {
    el_->DeleteEvent(this);
    SetTime(el_->Now() + interval_);
    el_->AddEvent(this);
  }
}

void PeriodicTimerEvent::Start(EventLoop *el) {
  el_ = el ? el : el_;
  if (!el_) {
    return;
  }
  running_ = true;
  SetTime(el_->Now() + interval_);
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
  timermanager_ = std::make_shared<TimerManager>();
  now_.SetNow();
  stop_ = true;
}

EventLoop::~EventLoop() {
  close(epfd_);
}

int EventLoop::CollectFileEvents(int timeout) {
  return epoll_wait(epfd_, evs_, 256, timeout);
}

int EventLoop::DoTimeout() {
  int n = 0;
  TimerManager::TimerMap& timers_map = timermanager_->timers_;
  TimerManager::TimerMap::iterator iter = timers_map.begin();
  while (iter != timers_map.end()) {
    TimeVal tv = iter->first;
    //printf("EventLoop::DoTimeout, now: %ld, tv: %ld\n", now_.Seconds(), tv.Seconds());
    if (TimeVal::MsDiff(now_, tv) < 0) break;
    n++;
    TimerManager::TimerSet events_set = iter->second;
    TimerManager::TimerSet::iterator iter2;
    for (iter2 = events_set.begin(); iter2 != events_set.end(); ++iter2) {
      TimerEvent *e = *iter2;
      e->OnEvents(TimerEvent::TIMER);
    }
    timers_map.erase(iter);
    iter = timers_map.begin();
  }
  return n;
}

int EventLoop::ProcessEvents(int timeout) {
  int i, nt, n;

  n = CollectFileEvents(timeout);
  now_.SetNow();
  nt = DoTimeout();

  for(i = 0; i < n; i++) {
    IEvent *e = (IEvent *)evs_[i].data.ptr;
    uint32_t events = 0;
    if (evs_[i].events & EPOLLIN) events |= IOEvent::READ;
    if (evs_[i].events & EPOLLOUT) events |= IOEvent::WRITE;
    if (evs_[i].events & (EPOLLHUP | EPOLLERR)) events |= IOEvent::ERROR;
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
    now_.SetNow();

    if (timermanager_->timers_.size() > 0) {
      TimerManager::TimerMap::iterator iter = timermanager_->timers_.begin();
      TimeVal time = iter->first;
      int t = TimeVal::MsDiff(time, now_);
      if (t > 0 && timeout > t) timeout = t;
    }

    ProcessEvents(timeout);
  }
}

int EventLoop::AddEvent(IOEvent *e) {
  epoll_event ev = {0, {0}};
  uint32_t events = e->events_;

  ev.events = 0;
  if (events & IOEvent::READ) ev.events |= EPOLLIN;
  if (events & IOEvent::WRITE) ev.events |= EPOLLOUT;
  if (events & IOEvent::ERROR) ev.events |= EPOLLHUP | EPOLLRDHUP | EPOLLERR;
  ev.data.fd = e->fd_;
  ev.data.ptr = e;

  SetNonblocking(e->fd_);

  return epoll_ctl(epfd_, EPOLL_CTL_ADD, e->fd_, &ev);
}

int EventLoop::UpdateEvent(IOEvent *e) {
  epoll_event ev = {0, {0}};
  uint32_t events = e->events_;

  ev.events = 0;
  if (events & IOEvent::READ) ev.events |= EPOLLIN;
  if (events & IOEvent::WRITE) ev.events |= EPOLLOUT;
  if (events & IOEvent::ERROR) ev.events |= EPOLLHUP | EPOLLRDHUP | EPOLLERR;
  ev.data.fd = e->fd_;
  ev.data.ptr = e;

  return epoll_ctl(epfd_, EPOLL_CTL_MOD, e->fd_, &ev);
}

int EventLoop::DeleteEvent(IOEvent *e) {
  epoll_event ev; // kernel before 2.6.9 requires
  return epoll_ctl(epfd_, EPOLL_CTL_DEL, e->fd_, &ev);
}

int EventLoop::AddEvent(TimerEvent *e) {
  e->el_ = this;
  return timermanager_->AddEvent(e);
}

int EventLoop::UpdateEvent(TimerEvent *e) {
  return timermanager_->UpdateEvent(e);
}

int EventLoop::DeleteEvent(TimerEvent *e) {
  return timermanager_->DeleteEvent(e);
}

int EventLoop::AddEvent(SignalEvent *e) {
  e->el_ = this;
  return SignalManager::Instance()->AddEvent(e);
}

int EventLoop::DeleteEvent(SignalEvent *e) {
  return SignalManager::Instance()->DeleteEvent(e);
}

int EventLoop::UpdateEvent(SignalEvent *e) {
  return SignalManager::Instance()->UpdateEvent(e);
}

int EventLoop::AddEvent(BufferIOEvent *e) {
  e->el_ = this;
  return AddEvent(dynamic_cast<IOEvent *>(e));
}

int EventLoop::AddEvent(PeriodicTimerEvent *e) {
  e->el_ = this;
  return AddEvent(dynamic_cast<TimerEvent *>(e));
}

void SignalHandler(int signo) {
  std::set<SignalEvent *> events = SignalManager::Instance()->sig_events_[signo];
  for (std::set<SignalEvent *>::iterator iter = events.begin(); iter != events.end(); ++iter) {
    (*iter)->OnEvents(signo);
  }
}

int SignalManager::AddEvent(SignalEvent *e) {
  struct sigaction action;
  action.sa_handler = SignalHandler;
  action.sa_flags = SA_RESTART;
  sigemptyset(&action.sa_mask);

  sig_events_[e->Signal()].insert(e);
  sigaction(e->Signal(), &action, NULL);

  return 0;
}

int SignalManager::DeleteEvent(SignalEvent *e) {
  sig_events_[e->Signal()].erase(e);
  return 0;
}

int SignalManager::UpdateEvent(SignalEvent *e) {
  sig_events_[e->Signal()].erase(e);
  sig_events_[e->Signal()].insert(e);
  return 0;
}

void IOEvent::AddWriteEvent() {
  if (el_ && !(events_ & IOEvent::WRITE))
  {
    SetEvents(events_ | IOEvent::WRITE);
    el_->UpdateEvent(this);
  }
}
void IOEvent::DeleteWriteEvent() {
  if (el_ && (events_ & IOEvent::WRITE))
  {
    SetEvents(events_ & (~IOEvent::WRITE));
    el_->UpdateEvent(this);
  }
}

// BufferIOEvent implementation
void BufferIOEvent::ClearBuff() {
  rx_msg_->Clear();
  tx_msg_mq_.clear();
}
bool BufferIOEvent::TxBuffEmpty() {
  return tx_msg_mq_.empty();
}

int BufferIOEvent::ReceiveData() {
  char buffer[MAX_BYTES_RECEIVE];
  int read_bytes = std::min(rx_msg_->MoreSize(), (size_t)sizeof(buffer));
  int len = read(fd_, buffer, read_bytes);
  //printf("[BufferIOEvent::ReceiveData] read_bytes: %d, got: %d\n", read_bytes, len);
  if (len < 0) {
    OnError(errno, strerror(errno));
  }
  else if (len == 0 ) {
    OnClosed();
  } else {
    size_t feeds = rx_msg_->AppendData(buffer, len);
    UNUSED(feeds);
    //printf("[BufferIOEvent::ReceiveData] Received bytes: %d, Feeds %d bytes of AppendData, rx_msg_ size: %d, msg length: %d\n",
    //    len, feeds, rx_msg_->Size(), dynamic_cast<BinaryMessage*>(rx_msg_.get())->Header()->length);

    if (rx_msg_->Completion()) {
      OnReceived(rx_msg_.get());
      rx_msg_->Clear();
    }
  }
  return len;
}

int BufferIOEvent::SendData() {
  uint32_t cur_sent = 0;
  while (!tx_msg_mq_.empty()) {
    const MessagePtr& tx_msg = tx_msg_mq_.front();
    uint32_t tosend = tx_msg->Size();
    int len = write(fd_, tx_msg->Data().data() + sent_, tosend - sent_);
    if (len < 0) {
      OnError(errno, strerror(errno));
      break;
    }
    sent_ += len;
    cur_sent += len;
    if (sent_ == tosend) {
      OnSent(tx_msg.get());
      tx_msg_mq_.pop_front();
      sent_ = 0;
    } else {
      /// sent_ less than tosend, breaking the sending loop and wait for next writing event
      break;
    }
  }
  if (tx_msg_mq_.empty()) {
    DeleteWriteEvent();  // All data in the output buffer has been sent, then remove writing event from epoll
  }
  return cur_sent;
}

void BufferIOEvent::OnEvents(uint32_t events) {
  /// The WRITE events should deal with before the READ events
  if (events & IOEvent::WRITE) {
    SendData();
  }
  if (events & IOEvent::READ) {
    ReceiveData();
  }
  if (events & IOEvent::ERROR) {
    OnError(errno, strerror(errno));
  }
}

void BufferIOEvent::Send(const Message& msg) {
  //MessagePtr msg_ptr = std::make_shared<Message>(msg);
  MessagePtr msg_ptr = CreateMessage(msg);
#ifndef _BINARY_MSG_MINIMUM_PACKAGING
  if (msg_type_ == MessageType::BINARY) {
    BinaryMessage* bmsg = static_cast<BinaryMessage*>(msg_ptr.get());
    bmsg->Header()->msg_id = ++msg_seq_;
  }
#endif
  SendInner(msg_ptr);
}

void BufferIOEvent::Send(const string& data) {
  Send(data.data(), data.size());
}

void BufferIOEvent::Send(const char *data, uint32_t len) {
  //MessagePtr msg_ptr = std::make_shared<Message>(data, len, Message::HAS_NO_HDR);
  MessagePtr msg_ptr = CreateMessage(msg_type_, data, len);
#ifndef _BINARY_MSG_MINIMUM_PACKAGING
  if (msg_type_ == MessageType::BINARY) {
    BinaryMessage* bmsg = static_cast<BinaryMessage*>(msg_ptr.get());
    bmsg->Header()->msg_id = ++msg_seq_;
    printf("[BufferIOEvent::Send] msg_header{ length: %d, msg_id: %d }\n",
        bmsg->Size(), bmsg->Header()->msg_id);
  }
#else
  printf("[BufferIOEvent::Send] msg_header{ length: %d }\n", msg_ptr->Size());
#endif
  SendInner(msg_ptr);
}

void BufferIOEvent::SendInner(const MessagePtr& msg) {
  tx_msg_mq_.push_back(msg);
  if (!(events_ & IOEvent::WRITE)) {
    AddWriteEvent();  // The output buffer has data now, then add writing event to epoll again if epoll has no writing event
  }
}

}   // ns evt_loop

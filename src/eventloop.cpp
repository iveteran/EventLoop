#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "eventloop.h"
#include "timer_handler.h"
#include "signal_handler.h"
#include "fd_handler.h"

namespace evt_loop {

time_t Now()
{
  return EV_Singleton->UnixTime();
}

int SetNonblocking(int fd) {
  if (fd < 0) return -1;
  int opts;
  if ((opts = fcntl(fd, F_GETFL)) != -1) {
    opts = opts | O_NONBLOCK;
    if(fcntl(fd, F_SETFL, opts) != -1) {
      return 0;
    }
  }
  return -1;
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
    if (e) {
      uint32_t events = 0;
      if (evs_[i].events & EPOLLIN) events |= IOEvent::READ;
      if (evs_[i].events & EPOLLOUT) events |= IOEvent::WRITE;
      if (evs_[i].events & (EPOLLHUP | EPOLLERR)) events |= IOEvent::ERROR;
      e->OnEvents(events);
    }
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

int EventLoop::SetEvent(IOEvent *e, int op)
{
  if (e->fd_ < 0) return -1;
  epoll_event ev = {0, {0}};
  uint32_t events = e->events_;

  ev.events = 0;
  if (events & IOEvent::READ) ev.events |= EPOLLIN;
  if (events & IOEvent::WRITE) ev.events |= EPOLLOUT;
  if (events & IOEvent::ERROR) ev.events |= EPOLLHUP | EPOLLRDHUP | EPOLLERR;
  ev.data.fd = e->fd_;
  ev.data.ptr = e;

  return epoll_ctl(epfd_, op, e->fd_, &ev);
}

int EventLoop::AddEvent(IOEvent *e) {
  e->el_ = this;
  SetNonblocking(e->fd_);
  return SetEvent(e, EPOLL_CTL_ADD);
}

int EventLoop::UpdateEvent(IOEvent *e) {
  return SetEvent(e, EPOLL_CTL_MOD);
}

int EventLoop::DeleteEvent(IOEvent *e) {
  if (e->fd_ < 0) return -1;
  epoll_event ev = {0, {0}}; // kernel before 2.6.9 requires
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

}   // ns evt_loop

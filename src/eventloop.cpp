#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "eventloop.h"
#include "timer_handler.h"
#include "signal_handler.h"
#include "fd_handler.h"
#include "user_event_handler.h"

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
  poller_ = std::make_shared<Poller>();
  timermanager_ = std::make_shared<TimerManager>();
  idle_events_ = std::make_shared<UserEventManager>();
  tick_events_ = std::make_shared<UserEventManager>();
  now_.SetNow();
  running_ = false;
  signal(SIGPIPE, SIG_IGN);  // Ignore SIGPIPE, this signal will be received when write the socket that closed by peer
}

EventLoop::~EventLoop() {
}

int EventLoop::ProcessFileEvents(int timeout) {
  return poller_->Poll(timeout, std::bind(&EventLoop::_ProcessFileEvents, this,
              std::placeholders::_1, std::placeholders::_2));
}

int EventLoop::ProcessTimeoutEvents() {
  int n = 0;
  TimerManager::TimerMap& timers_map = timermanager_->timers_;
  TimerManager::TimerMap::iterator iter = timers_map.begin();
  while (iter != timers_map.end()) {
    TimeVal tv = iter->first;
    //printf("EventLoop::ProcessTimeoutEvents, now: %d, tv: %d\n", now_.Seconds(), tv.Seconds());
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
  now_.SetNow();
  int timeout_events = ProcessTimeoutEvents();

  int file_events = ProcessFileEvents(timeout);

  int idle_events = 0;
  if (timeout_events == 0 && file_events == 0) {
    idle_events = ProcessIdleEvents();
  }

  int tick_events = ProcessTickEvents();

  return timeout_events + file_events + idle_events + tick_events;
}

void EventLoop::_ProcessFileEvents(void* evt, uint32_t events) {
  IOEvent* e = (IOEvent*)evt;
  if (e && e->fd_ > 0) {
    e->OnEvents(events);
  }
}

int EventLoop::ProcessIdleEvents()
{
  return idle_events_->Process();
}

int EventLoop::ProcessTickEvents()
{
  return tick_events_->Process();
}

int EventLoop::CalcNextTimeout()
{
    int timeout = 100;
    if (timermanager_->timers_.size() > 0) {
      TimerManager::TimerMap::iterator iter = timermanager_->timers_.begin();
      TimeVal time = iter->first;
      int t = TimeVal::MsDiff(time, now_);
      if (t > 0 && timeout > t) timeout = t;
    }
    return timeout;
}

void EventLoop::StopLoop() {
  running_ = false;
}

void EventLoop::StartLoop() {
  if (running_) {
    printf("Error: EventLoop already running\n");
    return;
  }

  running_ = true;
  while (running_) {
    now_.SetNow();
    int timeout = CalcNextTimeout();

    ProcessEvents(timeout);
  }
}

int EventLoop::AddEvent(IOEvent *e) {
  e->el_ = this;
  SetNonblocking(e->fd_);
  return poller_->SetEvents(e->fd_, PollerCtrl::ADD, e->events_, e);
}

int EventLoop::UpdateEvent(IOEvent *e) {
  return poller_->SetEvents(e->fd_, PollerCtrl::UPDATE, e->events_, e);
}

int EventLoop::DeleteEvent(IOEvent *e) {
  return poller_->SetEvents(e->fd_, PollerCtrl::DELETE, e->events_);
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

int EventLoop::AddEvent(IdleEvent *e) {
  e->el_ = this;
  return idle_events_->AddEvent(e);
}

int EventLoop::UpdateEvent(IdleEvent *e) {
  return idle_events_->UpdateEvent(e);
}

int EventLoop::DeleteEvent(IdleEvent *e) {
  return idle_events_->DeleteEvent(e);
}

int EventLoop::AddEvent(TickEvent *e) {
  e->el_ = this;
  return tick_events_->AddEvent(e);
}

int EventLoop::UpdateEvent(TickEvent *e) {
  return tick_events_->UpdateEvent(e);
}

int EventLoop::DeleteEvent(TickEvent *e) {
  return tick_events_->DeleteEvent(e);
}

}   // ns evt_loop

#include "timer_handler.h"
#include "eventloop.h"
#include "logger.h"

namespace evt_loop
{

// TimerEvent implementation
TimerEvent::TimerEvent() :
  IEvent(IEvent::NONE), running_(false)
{
  el_ = EV_Singleton;
}
TimerEvent::TimerEvent(const TimeVal& inter) :
  IEvent(IEvent::NONE), interval_(inter), running_(false)
{
  el_ = EV_Singleton;
}

void TimerEvent::OnEvents(uint32_t events, void* ctx) {
  el_logger->debug("[TimerEvent::OnEvents] timeval: ({}.{})", interval_.Seconds(), interval_.USeconds());
  OnTimer();
  if (running_) {
    el_->DeleteEvent(this);
    SetTime(el_->Now() + interval_);
    el_->AddEvent(this);
  }
}

void TimerEvent::Start(bool immediately) {
  if (!el_) return;
  running_ = true;
  el_logger->debug("[TimerEvent::Start] timeval: ({}.{})", interval_.Seconds(), interval_.USeconds());
  SetTime(el_->Now() + interval_);
  el_->AddEvent(this);
  if (immediately) {
    OnEvents(0);
  }
}

void TimerEvent::Stop() {
  el_logger->debug("[TimerEvent::Stop] Timer stopped");
  if (!el_) return;
  running_ = false;
  el_->DeleteEvent(this);
}

// TimerManager implementation
int TimerManager::AddEvent(TimerEvent *e) {
  //el_logger->debug("[TimerManager::AddEvent] event object: {}, timeval: ({}.{})", e, e->Time().tv_sec, e->Time().tv_usec);
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

}  // namespace evt_loop

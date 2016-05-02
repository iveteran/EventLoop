#include "timer_handler.h"
#include "eventloop.h"

namespace evt_loop
{

// PeriodicTimerEvent implementation
PeriodicTimerEvent::PeriodicTimerEvent() :
  TimerEvent(IEvent::NONE), running_(false)
{
  el_ = EV_Singleton;
}
PeriodicTimerEvent::PeriodicTimerEvent(const TimeVal& inter) :
  TimerEvent(IEvent::NONE), interval_(inter), running_(false)
{
  el_ = EV_Singleton;
}

void PeriodicTimerEvent::OnEvents(uint32_t events) {
  OnTimer();
  if (running_) {
    el_->DeleteEvent(this);
    SetTime(el_->Now() + interval_);
    el_->AddEvent(this);
  }
}

void PeriodicTimerEvent::Start() {
  if (!el_) return;
  running_ = true;
  SetTime(el_->Now() + interval_);
  el_->AddEvent(this);
}

void PeriodicTimerEvent::Stop() {
  if (!el_) return;
  running_ = false;
  el_->DeleteEvent(this);
}

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

}  // namespace evt_loop

#include "timer_handler.h"
#include "eventloop.h"

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

void TimerEvent::OnEvents(uint32_t events) {
  printf("[TimerEvent::OnEvents] timeval: (%d.%d)\n", interval_.Seconds(), interval_.USeconds());
  OnTimer();
  if (running_) {
    el_->DeleteEvent(this);
    SetTime(el_->Now() + interval_);
    el_->AddEvent(this);
  }
}

void TimerEvent::Start() {
  if (!el_) return;
  running_ = true;
  printf("[TimerEvent::Start] timeval: (%d.%d)\n", interval_.Seconds(), interval_.USeconds());
  SetTime(el_->Now() + interval_);
  el_->AddEvent(this);
}

void TimerEvent::Stop() {
  printf("[TimerEvent::Stop] Timer stopped\n");
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

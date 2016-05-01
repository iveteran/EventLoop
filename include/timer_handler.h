#ifndef _TIMER_HANDLER_H
#define _TIMER_HANDLER_H

#include <set>
#include <map>
#include <functional>
#include "event.h"
#include "utils.h"

namespace evt_loop
{

class TimerEvent : public IEvent {
  friend class EventLoop;
 public:
  static const uint32_t TIMER = 1 << 0;

 public:
  TimerEvent(uint32_t events = IEvent::NONE) : IEvent(events) {}

  void SetTime(const TimeVal& tv) { time_ = tv; }
  const TimeVal& Time() const { return time_; }

 private:
  TimeVal time_;
};

class PeriodicTimerEvent : public TimerEvent {
 public:
  PeriodicTimerEvent() : TimerEvent(IEvent::NONE), running_(false) {};
  PeriodicTimerEvent(const TimeVal& inter) : TimerEvent(IEvent::NONE), interval_(inter), running_(false) {};

  void SetInterval(const TimeVal& inter) { interval_ = inter; }
  const TimeVal& GetInterval() const { return interval_; }

  void Start(EventLoop* el = NULL);
  void Stop();

  bool IsRunning() { return running_; }

  virtual void OnTimer() = 0;

 private:
  void OnEvents(uint32_t events);

 private:
  TimeVal   interval_;
  bool      running_;
};

class PeriodicTimer : public PeriodicTimerEvent {
  typedef std::function<void (PeriodicTimer*)>  OnTimerCallback;
  public:
    PeriodicTimer(const OnTimerCallback& cb) : timer_cb_(cb) { }
    void OnTimer() { timer_cb_(this); }

  private:
    OnTimerCallback timer_cb_;
};

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

}  // namepace evt_loop

#endif  // _TIMER_HANDLER_H

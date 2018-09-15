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
  static const uint32_t TIMER = 1 << 0;

 public:
  TimerEvent();
  TimerEvent(const TimeVal& inter);

  void SetTime(const TimeVal& tv) { time_ = tv; }
  const TimeVal& Time() const { return time_; }

  void SetInterval(const TimeVal& inter) { interval_ = inter; }
  const TimeVal& GetInterval() const { return interval_; }

  void Start(bool immediately = false);
  void Stop();

  bool IsRunning() { return running_; }

 protected:
  void OnEvents(uint32_t events) override;
  virtual void OnTimer() = 0;

 protected:
  TimeVal   time_;
  TimeVal   interval_;
  bool      running_;
};

class PeriodicTimer : public TimerEvent {
 public:
  typedef std::function<void (TimerEvent*)>  OnTimerCallback;

  PeriodicTimer(const OnTimerCallback& cb) : timer_cb_(cb) { }
  PeriodicTimer(const TimeVal& inter, const OnTimerCallback& cb) : TimerEvent(inter), timer_cb_(cb) { }

 protected:
  void OnTimer() override { timer_cb_(this); }

 protected:
  OnTimerCallback timer_cb_;
};

class OneshotTimer : public PeriodicTimer {
 public:
  OneshotTimer(const OnTimerCallback& cb) : PeriodicTimer(cb) { }
  OneshotTimer(const TimeVal& inter, const OnTimerCallback& cb) : PeriodicTimer(inter, cb) { }

 protected:
  void OnTimer() override { PeriodicTimer::OnTimer(); Stop(); }
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

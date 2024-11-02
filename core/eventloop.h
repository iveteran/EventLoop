#ifndef _EVENT_LOOP_H
#define _EVENT_LOOP_H

#include <memory>
#include "utils.h"
#include "poller.h"

namespace evt_loop {

class IOEvent;
class BufferIOEvent;
class TimerManager;
class SignalEvent;
class TimerEvent;
class PeriodicTimerEvent;
class IdleEvent;
class TickEvent;
class UserEventManager;

time_t Now();
int64_t NowMilliSeconds();
int64_t NowMicroSeconds();
int SetNonblocking(int fd);

const int TICK_MS_MIN = 10;   // 10 milliseconds
const int TICK_MS_MID = 20;
const int TICK_MS_DFT = 50;
const int TICK_MS_MAX = 100;

class EventLoop {
 public:
  EventLoop(int tick_ms = TICK_MS_DFT);
  ~EventLoop();

 public:
  // add delete & update event objects
  int AddEvent(IOEvent *e);
  int DeleteEvent(IOEvent *e);
  int UpdateEvent(IOEvent *e);

  int AddEvent(TimerEvent *e);
  int DeleteEvent(TimerEvent *e);
  int UpdateEvent(TimerEvent *e);

  int AddEvent(SignalEvent *e);
  int DeleteEvent(SignalEvent *e);
  int UpdateEvent(SignalEvent *e);

  int AddEvent(IdleEvent *e);
  int DeleteEvent(IdleEvent *e);
  int UpdateEvent(IdleEvent *e);

  int AddEvent(TickEvent *e);
  int DeleteEvent(TickEvent *e);
  int UpdateEvent(TickEvent *e);

  // event loop control
  void StartLoop();
  void StopLoop();

  void SetTickMS(int ms) { if (ms > 0) tick_ms_ = ms; }
  int GetTickMS() const { return tick_ms_; }
  bool IsRunning() const { return running_; }
  const TimeVal& Now() const { return now_; }
  time_t UnixTime() const { return now_.Seconds(); }
  int64_t MilliSeconds() const { return now_.Seconds() * 1000 + now_.USeconds() / 1000; }
  int64_t MicroSeconds() const { return now_.Seconds() * 1000000 + now_.USeconds(); }

  std::shared_ptr<Poller> GetPoller() const {
      return poller_;
  }

 private:
  // do epoll_waite and collect events
  int ProcessEvents(int timeout);

  int ProcessFileEvents(int timeout);
  int ProcessTimeoutEvents();
  int ProcessIdleEvents();
  int ProcessTickEvents();
  void _ProcessFileEvents(IOEvent* e, uint32_t events, void* events_ctx = nullptr);

  int CalcNextTimeout();

 private:
  std::shared_ptr<Poller>   poller_;

  TimeVal   now_;
  bool      running_;
  int       tick_ms_;

  std::shared_ptr<TimerManager> timermanager_;
  std::shared_ptr<UserEventManager> idle_events_;
  std::shared_ptr<UserEventManager> tick_events_;
};

}  // ns evt_loop

#include "singleton_tmpl.h"
#define EV_Singleton       (Singleton<EventLoop>::GetInstance())

#endif // _EVENT_LOOP_H

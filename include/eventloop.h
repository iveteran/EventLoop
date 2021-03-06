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
int SetNonblocking(int fd);

class EventLoop {
 public:
  EventLoop();
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

  bool IsRunning() const { return running_; }
  const TimeVal& Now() const { return now_; }
  time_t UnixTime() const { return now_.Seconds(); }

 private:
  // do epoll_waite and collect events
  int ProcessEvents(int timeout);

  int ProcessFileEvents(int timeout);
  int ProcessTimeoutEvents();
  int ProcessIdleEvents();
  int ProcessTickEvents();
  void _ProcessFileEvents(void* evt, uint32_t events);

  int CalcNextTimeout();

 private:
  std::shared_ptr<Poller>   poller_;

  TimeVal   now_;
  bool      running_;

  std::shared_ptr<TimerManager> timermanager_;
  std::shared_ptr<UserEventManager> idle_events_;
  std::shared_ptr<UserEventManager> tick_events_;
};

}  // ns evt_loop

#include "singleton_tmpl.h"
#define EV_Singleton       (Singleton<EventLoop>::GetInstance())

#endif // _EVENT_LOOP_H

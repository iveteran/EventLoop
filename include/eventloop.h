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
class IdleEventManager;

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

  // do epoll_waite and collect events
  int ProcessEvents(int timeout);
  void ProcessFileEvents(void* evt, uint32_t events);
  int ProcessIdleEvents();

  int CalcNextTimeout();

  // event loop control
  void StartLoop();
  void StopLoop();

  bool IsRunning() const { return running_; }
  const TimeVal& Now() const { return now_; }
  time_t UnixTime() const { return now_.Seconds(); }

 private:
  int PollFileEvents(int timeout);
  int DoTimeout();

 private:
  Poller    poller_;

  TimeVal   now_;
  bool      running_;

  std::shared_ptr<TimerManager> timermanager_;
  std::shared_ptr<IdleEventManager> idle_events_;
};

}  // ns evt_loop

#include "singleton_tmpl.h"
#define EV_Singleton       (Singleton<EventLoop>::GetInstance())

#endif // _EVENT_LOOP_H

#ifndef _EVENT_LOOP_H
#define _EVENT_LOOP_H

#include <sys/epoll.h>
#include <memory>
#include "utils.h"

namespace evt_loop {

class IOEvent;
class BufferIOEvent;
class TimerManager;
class SignalEvent;
class TimerEvent;
class PeriodicTimerEvent;

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

  int AddEvent(BufferIOEvent *e);
  int AddEvent(PeriodicTimerEvent *e);

  // do epoll_waite and collect events
  int ProcessEvents(int timeout);

  // event loop control
  void StartLoop();
  void StopLoop();

  const TimeVal& Now() const { return now_; }
  time_t UnixTime() const { return now_.Seconds(); }

 private:
  int SetEvent(IOEvent *e, int op);
  int CollectFileEvents(int timeout);
  int DoTimeout();

 private:
  int epfd_;
  epoll_event evs_[256];

  TimeVal   now_;
  bool      stop_;

  std::shared_ptr<TimerManager> timermanager_;
};

time_t Now();
int SetNonblocking(int fd);

}  // ns evt_loop

#include "singleton_tmpl.h"
#define EV_Singleton       (Singleton<EventLoop>::GetInstance())

#endif // _EVENT_LOOP_H

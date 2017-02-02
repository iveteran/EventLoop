#ifndef _IDLE_HANDLER_H
#define _IDLE_HANDLER_H

#include <set>
#include <map>
#include <functional>
#include "event.h"
#include "utils.h"

namespace evt_loop
{

class IdleEvent : public IEvent {
  friend class EventLoop;
  friend class IdleEventManager;
  typedef std::function<void (IdleEvent*, void* udata)> OnIdleCallback;
  typedef std::function<void (uint32_t eventid)>        OnFinishCallback;

 public:
  IdleEvent(const OnIdleCallback& cb, void* udata = NULL, int32_t repeat = -1);
  uint32_t Id() const { return id_; }

 protected:
  void OnEvents(uint32_t events) override;

 private:
  uint32_t  id_;
  OnIdleCallback idle_cb_;
  void*     user_data_;
  int32_t   repeat_;
  //OnFinishCallback  finish_cb_;
};

class IdleEventManager {
  friend class EventLoop;
  typedef std::map<uint32_t, IdleEvent*> IdleEvents;

 public:
  uint32_t Process();
  int AddEvent(IdleEvent *e);
  int DeleteEvent(IdleEvent *e);
  int UpdateEvent(IdleEvent *e);

 private:
  uint32_t last_eventid_;
  IdleEvents idle_events_;
};

}  // namepace evt_loop

#endif  // _IDLE_HANDLER_H

#ifndef _USER_EVENT_HANDLER_H
#define _USER_EVENT_HANDLER_H

#include <set>
#include <map>
#include <functional>
#include "event.h"
#include "utils.h"

namespace evt_loop
{

class UserEvent : public IEvent {
  friend class EventLoop;
  friend class UserEventManager;
 public:
  typedef std::function<void (UserEvent*, void* udata)> OnUserEventCallback;
  typedef std::function<void (uint32_t eventid)>        OnFinishCallback;

 public:
  UserEvent(const OnUserEventCallback& cb, void* udata = NULL, int32_t repeat = -1);
  uint32_t Id() const { return id_; }

 protected:
  void OnEvents(uint32_t events) override;

 protected:
  uint32_t  id_;
  OnUserEventCallback user_event_cb_;
  void*     user_data_;
  int32_t   repeat_;
  //OnFinishCallback  finish_cb_;
};

class IdleEvent : public UserEvent {
  public:
  IdleEvent(const OnUserEventCallback& cb, void* udata = NULL, int32_t repeat = -1);
 protected:
  void OnEvents(uint32_t events) override;
};

class TickEvent : public UserEvent {
  public:
  TickEvent(const OnUserEventCallback& cb, void* udata = NULL, int32_t repeat = -1);
 protected:
  void OnEvents(uint32_t events) override;
};

class UserEventManager {
  friend class EventLoop;
  typedef std::map<uint32_t, UserEvent*> UserEvents;

 public:
  uint32_t Process();
  int AddEvent(UserEvent *e);
  int DeleteEvent(UserEvent *e);
  int UpdateEvent(UserEvent *e);

 private:
  uint32_t last_eventid_;
  UserEvents user_events_;
};

}  // namepace evt_loop

#endif  // _USER_EVENT_HANDLER_H

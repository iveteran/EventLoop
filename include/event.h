#ifndef _EVENT_H
#define _EVENT_H

#include <stdint.h>
#include <string.h>

namespace evt_loop
{

class EventLoop;

class IEvent {
 public:
  static const uint32_t  NONE = 0;
  static const uint32_t  ONESHOT = 1 << 30;
  static const uint32_t  TIMEOUT = 1 << 31;

 public:
  IEvent(uint32_t events = 0, EventLoop* el = NULL) : events_(events), el_(el) { }
  virtual ~IEvent() {};

  virtual void OnEvents(uint32_t events) = 0;

 public:
  virtual void SetEvents(uint32_t events) { events_ = events; }
  virtual uint32_t Events() const { return events_; }

 protected:
  uint32_t events_;
  EventLoop *el_;
};

}  // namespace evt_loop

#endif  // _EVENT_H

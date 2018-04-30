#include "user_event_handler.h"
#include "eventloop.h"

namespace evt_loop
{

UserEvent::UserEvent(const OnUserEventCallback& cb, void* udata, int32_t repeat) :
  IEvent(IEvent::NONE), id_(0), user_event_cb_(cb), user_data_(udata), repeat_(repeat)
{
  el_ = EV_Singleton;
}

void UserEvent::OnEvents(uint32_t events) {
  user_event_cb_(this, user_data_);
}

IdleEvent::IdleEvent(const OnUserEventCallback& cb, void* udata, int32_t repeat) :
  UserEvent(cb, udata, repeat)
{
  el_->AddEvent(this);
}

TickEvent::TickEvent(const OnUserEventCallback& cb, void* udata, int32_t repeat) :
  UserEvent(cb, udata, repeat)
{
  el_->AddEvent(this);
}

void IdleEvent::OnEvents(uint32_t events) {
  UserEvent::OnEvents(events);
  if (--repeat_ == 0)
    el_->DeleteEvent(this);
}

void TickEvent::OnEvents(uint32_t events) {
  UserEvent::OnEvents(events);
  if (--repeat_ == 0)
    el_->DeleteEvent(this);
}

uint32_t UserEventManager::Process()
{
  uint32_t num = 0;
  for (auto iter = user_events_.begin(); iter != user_events_.end(); ++iter, ++num) {
    iter->second->OnEvents(1);
  }
  return num;
}

int UserEventManager::AddEvent(UserEvent *e) {
  if (e->id_ == 0)
    e->id_ = ++last_eventid_;
  user_events_[e->id_] = e;
  return 0;
}

int UserEventManager::DeleteEvent(UserEvent *e) {
  user_events_.erase(e->id_);
  return 0;
}

int UserEventManager::UpdateEvent(UserEvent *e) {
  DeleteEvent(e);
  AddEvent(e);
  return 0;
}

}  // namespace evt_loop

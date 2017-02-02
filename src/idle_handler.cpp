#include "idle_handler.h"
#include "eventloop.h"

namespace evt_loop
{

IdleEvent::IdleEvent(const OnIdleCallback& cb, void* udata, int32_t repeat) :
  IEvent(IEvent::NONE), id_(0), idle_cb_(cb), user_data_(udata), repeat_(repeat)
{
  el_ = EV_Singleton;
  el_->AddEvent(this);
}

void IdleEvent::OnEvents(uint32_t events) {
  idle_cb_(this, user_data_);
  if (--repeat_ == 0)
    el_->DeleteEvent(this);
}

uint32_t IdleEventManager::Process()
{
  uint32_t num = 0;
  for (auto iter = idle_events_.begin(); iter != idle_events_.end(); ++iter, ++num) {
    iter->second->OnEvents(1);
  }
  return num;
}

int IdleEventManager::AddEvent(IdleEvent *e) {
  if (e->id_ == 0)
    e->id_ = ++last_eventid_;
  idle_events_[e->id_] = e;
  return 0;
}

int IdleEventManager::DeleteEvent(IdleEvent *e) {
  idle_events_.erase(e->id_);
  return 0;
}

int IdleEventManager::UpdateEvent(IdleEvent *e) {
  DeleteEvent(e);
  AddEvent(e);
  return 0;
}

}  // namespace evt_loop

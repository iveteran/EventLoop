#include "user_event_handler.h"
#include "eventloop.h"

namespace evt_loop
{

UserEvent::UserEvent(const OnUserEventCallback& cb, void* udata, int32_t repeat) :
  IEvent(IEvent::NONE), id_(0), user_event_cb_(cb), user_data_(udata), repeat_(repeat),
  soft_deleting_enabled_(false), soft_deleted_(false)
{
  el_ = EV_Singleton;
}

void UserEvent::OnEvents(uint32_t events, void* ctx) {
  user_event_cb_(this, user_data_);
  if (repeat_ > 0 && --repeat_ == 0) {
    if (soft_deleting_enabled_) {
      soft_deleted_ = true;
    } else {
      DeleteEvents();
    }
  }
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

void IdleEvent::DeleteEvents() {
    el_->DeleteEvent(this);
}

void TickEvent::DeleteEvents() {
    el_->DeleteEvent(this);
}

uint32_t UserEventManager::Process()
{
  uint32_t num = 0;
  for (auto iter = user_events_.begin(); iter != user_events_.end(); ++num) {
    auto e = iter->second;
    e->soft_deleting_enabled_ = true;
    e->OnEvents(1);
    if (e->soft_deleted_) {
      iter = ++iter;
      DeleteEvent(e);
    } else {
      e->soft_deleting_enabled_ = false;
      ++iter;
    }
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

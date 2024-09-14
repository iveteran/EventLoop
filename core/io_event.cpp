#include "io_event.h"
#include "eventloop.h"

namespace evt_loop
{

bool ValidFD(int fd) {
  return fd >= 0;
}

IOEvent::IOEvent(IOType type, int fd, uint32_t events) :
  IEvent(events), type_(type), fd_(fd)
{
  if (ValidFD(fd_)) {
    EV_Singleton->AddEvent(this);
  }
}
IOEvent::~IOEvent() {
  //printf("[IOEvent::~IOEvent] addr: %p, type: %d, fd: %d\n", this, type_, fd_);
  if (ValidFD(fd_)) {
    EV_Singleton->DeleteEvent(this);
    close(fd_);
    fd_ = -1;
  }
}
void IOEvent::SetFD(int fd) {
  if (fd != fd_) {
    if (!ValidFD(fd)) {
      EV_Singleton->DeleteEvent(this);
      fd_ = fd;
    } else {
      // for update fd
      if (ValidFD(fd_)) {
        EV_Singleton->DeleteEvent(this);
      }
      fd_ = fd;
      EV_Singleton->AddEvent(this);
    }
  }
}
void IOEvent::WatchEvents(int fd, uint32_t events)
{
  SetEvents(events);
  SetFD(fd);
}
void IOEvent::UpdateEvents(uint32_t events)
{
  if (ValidFD(fd_)) {
    SetEvents(events);
    if (el_) {
      el_->UpdateEvent(this);
    } else {
      EV_Singleton->AddEvent(this);
    }
  }
}
void IOEvent::AddReadEvent() {
  if (el_ && !(events_ & FileEvent::READ))
  {
    SetEvents(events_ | FileEvent::READ);
    el_->UpdateEvent(this);
  }
}
void IOEvent::DeleteReadEvent() {
  if (el_ && (events_ & FileEvent::READ))
  {
    SetEvents(events_ & (~FileEvent::READ));
    el_->UpdateEvent(this);
  }
}

void IOEvent::AddWriteEvent() {
  if (el_ && !(events_ & FileEvent::WRITE))
  {
    SetEvents(events_ | FileEvent::WRITE);
    el_->UpdateEvent(this);
  }
}
void IOEvent::DeleteWriteEvent() {
  if (el_ && (events_ & FileEvent::WRITE))
  {
    SetEvents(events_ & (~FileEvent::WRITE));
    el_->UpdateEvent(this);
  }
}

void IOEvent::AddErrorEvent() {
  if (el_ && !(events_ & FileEvent::ERROR))
  {
    SetEvents(events_ | FileEvent::ERROR);
    el_->UpdateEvent(this);
  }
}
void IOEvent::DeleteErrorEvent() {
  if (el_ && (events_ & FileEvent::ERROR))
  {
    SetEvents(events_ & (~FileEvent::ERROR));
    el_->UpdateEvent(this);
  }
}
void IOEvent::ClearAllEvents() {
  if (el_) {
    SetEvents(0);
    el_->DeleteEvent(this);
  }
}

}  // namespace evt_loop

#ifndef _IO_EVENT_H
#define _IO_EVENT_H

#include <unistd.h>
#include "event.h"
#include "poller.h"

namespace evt_loop
{

class IOEvent : public IEvent {
  friend class EventLoop;

 public:
  enum IOType { NONE, TCP_CLIENT, TCP_SERVER, TCP_CONNECTION, UDP_PEER, STDIN, COUNT };

 public:
  IOEvent(IOType type = IOType::NONE, int fd = -1, uint32_t events = FileEvent::READ | FileEvent::ERROR);
  virtual ~IOEvent();

 public:
  void SetFD(int fd);
  int FD() const { return fd_; }
  void WatchEvents(int fd, uint32_t events = FileEvent::READ | FileEvent::ERROR);
  void UpdateEvents(uint32_t events);
  void UpdateEvents();

  void AddReadEvent();
  void DeleteReadEvent();
  void AddWriteEvent();
  void DeleteWriteEvent();
  void AddErrorEvent();
  void DeleteErrorEvent();
  void ClearAllEvents();

 protected:
  virtual void OnCreated(int fd) {};
  virtual void OnClosed() {};
  virtual void OnError(int errcode, const char* errstr) {};

  virtual void OnEvents(uint32_t events, void* ctx = nullptr) = 0;
  virtual int OnRead(const void* buf, size_t bytes) { return read(fd_, (void*)buf, bytes); }
  virtual int OnWrite(const void* buf, size_t bytes) { return write(fd_, buf, bytes); }

 protected:
  IOType type_;
  int fd_;
};

}  // namespace evt_loop
#endif  // _IO_EVENT_H

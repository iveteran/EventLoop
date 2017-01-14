#ifndef _FD_HANDLER_H
#define _FD_HANDLER_H

#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include "event.h"
#include "message.h"

using std::string;

namespace evt_loop
{

class IOEvent : public IEvent {
  friend class EventLoop;
 public:
  static const uint32_t  READ = 1 << 0;
  static const uint32_t  WRITE = 1 << 1;
  static const uint32_t  ERROR = 1 << 2;
  static const uint32_t  CREATE = 1 << 3;
  static const uint32_t  CLOSED = 1 << 4;

 public:
  IOEvent(int fd = -1, uint32_t events = IOEvent::READ | IOEvent::ERROR);
  virtual ~IOEvent();

 public:
  void SetFD(int fd);
  int FD() const { return fd_; }
  void WatchEvents(int fd, uint32_t events = IOEvent::READ | IOEvent::ERROR);
  void UpdateEvents(uint32_t events);

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

  virtual void OnEvents(uint32_t events) = 0;
  virtual int OnRead(const void* buf, size_t bytes) { return read(fd_, (void*)buf, bytes); }
  virtual int OnWrite(const void* buf, size_t bytes) { return send(fd_, buf, bytes, MSG_NOSIGNAL); }

 protected:
  int fd_;
};

class BufferIOEvent : public IOEvent {
 public:
  enum State { CLOSED, CONNECTED, READY, HANDSHAKING, FAILED };

 public:
  BufferIOEvent(int fd, uint32_t events = IOEvent::READ | IOEvent::WRITE | IOEvent::ERROR)
    : IOEvent(fd, events), state_(CONNECTED), sent_(0), msg_seq_(0), close_wait_(false) {
  }

  State GetState() const { return state_; }

  void SetMessageType(const MessageType& msg_type) {
    msg_type_ = msg_type;
    rx_msg_mq_.Clear();
    rx_msg_mq_.SetMessageType(msg_type_);
    tx_msg_mq_.Clear();
    tx_msg_mq_.SetMessageType(msg_type_);
  }
  void ClearBuff();
  bool TxBuffEmpty();
  void Send(const Message& msg);
  void Send(const string& data, bool bmsg_has_hdr = BinaryMessage::HAS_NO_HDR);
  void Send(const char *data, uint32_t len, bool bmsg_has_hdr = BinaryMessage::HAS_NO_HDR);
  void SendMore(const string& data);
  void SendMore(const char *data, uint32_t len);
  void SetCloseWait() { close_wait_ = true; }

 protected:
  virtual void OnReceived(const Message* msg) { }
  virtual void OnSent(const Message* msg) { }
  virtual void OnReady() { }

  virtual void OnHandshake() { printf("BufferIOEvent::OnHandshake\n"); state_ = READY; OnReady(); }

 private:
  void OnEvents(uint32_t events);
  int ReceiveData(uint32_t& events);
  int SendData(uint32_t& events);
  void SendInner(const MessagePtr& msg);

 protected:
  State         state_;

 private:
  MessageType   msg_type_;
  MessageMQ     rx_msg_mq_;
  MessageMQ     tx_msg_mq_;
  uint32_t      sent_;
  uint32_t      msg_seq_;
  bool          close_wait_;

};

}  // namespace evt_loop

#endif  // _FD_HANDLER_H

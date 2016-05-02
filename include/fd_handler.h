#ifndef _FD_HANDLER_H
#define _FD_HANDLER_H

#include <string>
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

 protected:
  int fd_;
};

class BufferIOEvent : public IOEvent {
 public:
  BufferIOEvent(int fd, uint32_t events = IOEvent::READ | IOEvent::ERROR)
    : IOEvent(fd, events), sent_(0), msg_seq_(0) {
  }

 public:
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
  void Send(const string& data);
  void Send(const char *data, uint32_t len);

 protected:
  virtual void OnReceived(const Message* msg) { };
  virtual void OnSent(const Message* msg) { };

 private:
  void OnEvents(uint32_t events);
  int ReceiveData();
  int SendData();
  void SendInner(const MessagePtr& msg);

 private:
  MessageType   msg_type_;
  MessageMQ     rx_msg_mq_;
  MessageMQ     tx_msg_mq_;
  uint32_t      sent_;
  uint32_t      msg_seq_;

};

}  // namespace evt_loop

#endif  // _FD_HANDLER_H

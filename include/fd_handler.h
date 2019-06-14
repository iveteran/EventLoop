#ifndef _FD_HANDLER_H
#define _FD_HANDLER_H

#if defined(__OSX__) || defined(__DARWIN__) || defined(__APPLE__) || defined(__FREEBSD__)
#define MSG_NOSIGNAL MSG_HAVEMORE
#endif

#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include "event.h"
#include "message.h"
#include "poller.h"

using std::string;

namespace evt_loop
{

class IOEvent : public IEvent {
  friend class EventLoop;
  /*
 public:
  static const uint32_t  READ = 1 << 0;
  static const uint32_t  WRITE = 1 << 1;
  static const uint32_t  ERROR = 1 << 2;
  static const uint32_t  CREATE = 1 << 3;
  static const uint32_t  CLOSED = 1 << 4;
  */

 public:
  IOEvent(int fd = -1, uint32_t events = FileEvent::READ | FileEvent::ERROR);
  virtual ~IOEvent();

 public:
  void SetFD(int fd);
  int FD() const { return fd_; }
  void WatchEvents(int fd, uint32_t events = FileEvent::READ | FileEvent::ERROR);
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
  enum State { CLOSED, CONNECTED, READY, HANDSHAKING, FAILED, COUNT };

 public:
  BufferIOEvent(int fd, uint32_t events = FileEvent::READ | FileEvent::WRITE | FileEvent::ERROR)
    : IOEvent(fd, events), state_(CONNECTED), sent_(0), msg_seq_(0), close_wait_(false),
    stats_rx_bytes_(0), stats_rx_last_time_(0), stats_tx_bytes_(0), stats_tx_last_time_(0) {
  }

  State GetState() const { return state_; }

  void SetMessageType(const MessageType& msg_type) {
    msg_type_ = msg_type;
    rx_msg_mq_.Clear();
    rx_msg_mq_.SetMessageType(msg_type_);
    tx_msg_mq_.Clear();
    tx_msg_mq_.SetMessageType(msg_type_);
  }
  MessageType GetMessageType() const { return msg_type_; }
  void ClearBuff();
  bool TxBuffEmpty();
  bool Send(const Message& msg);
  bool Send(const string& data, bool bmsg_has_hdr = BinaryMessage::HAS_NO_HDR);
  bool Send(const char *data, uint32_t len, bool bmsg_has_hdr = BinaryMessage::HAS_NO_HDR);
  bool SendMore(const string& data);
  bool SendMore(const char *data, uint32_t len);
  void SetCloseWait() { close_wait_ = true; }

  uint32_t StatsRxBytes() const     { return stats_rx_bytes_; };
  time_t   StatsRxLastTime() const  { return stats_rx_last_time_; };
  uint32_t StatsTxBytes() const     { return stats_tx_bytes_; };
  time_t   StatsTxLastTime() const  { return stats_tx_last_time_; };

 protected:
  virtual void OnReceived(const Message* msg) { }
  virtual void OnSent(const Message* msg) { }
  virtual void OnReady() { }

  virtual bool OnHandshake() { printf("BufferIOEvent::OnHandshake\n"); state_ = READY; OnReady(); return true; }

 private:
  void OnEvents(uint32_t events);
  int ReceiveData(uint32_t& events);
  int SendData(uint32_t& events);
  bool SendInner(const MessagePtr& msg);

  void UpdateRxStats(uint32_t rx_bytes);
  void UpdateTxStats(uint32_t tx_bytes);

 protected:
  State         state_;

 private:
  MessageType   msg_type_;
  MessageMQ     rx_msg_mq_;
  MessageMQ     tx_msg_mq_;
  uint32_t      sent_;
  uint32_t      msg_seq_;
  bool          close_wait_;

  uint32_t      stats_rx_bytes_;
  time_t        stats_rx_last_time_;
  uint32_t      stats_tx_bytes_;
  time_t        stats_tx_last_time_;
};

}  // namespace evt_loop

#endif  // _FD_HANDLER_H

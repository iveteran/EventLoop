#ifndef _FD_HANDLER_H
#define _FD_HANDLER_H

#include <string>
#include <sys/socket.h>
#include "io_event.h"
#include "message.h"

#if defined(__OSX__) || defined(__DARWIN__) || defined(__APPLE__) || defined(__FREEBSD__)
#define MSG_NOSIGNAL MSG_HAVEMORE
#endif

using std::string;

namespace evt_loop
{

class BufferIOEvent : public IOEvent {
 public:
  enum State { CLOSED, CONNECTED, READY, HANDSHAKING, FAILED, COUNT };

 public:
  BufferIOEvent(IOType io_type, int fd, uint32_t events = FileEvent::READ | FileEvent::WRITE | FileEvent::ERROR)
    : IOEvent(io_type, fd, events), state_(CONNECTED), sent_(0), msg_seq_(0), close_wait_(false),
    stats_rx_bytes_(0), stats_rx_last_time_(0), stats_tx_bytes_(0), stats_tx_last_time_(0) {
  }
  virtual ~BufferIOEvent() { state_ = CLOSED; }

  State GetState() const { return state_; }

  void SetMessageType(const MessageType& msg_type, const HeaderDescriptionPtr& msg_hdr_desc) {
    msg_type_ = msg_type;
    msg_hdr_desc_ = msg_hdr_desc;
    rx_msg_mq_.Clear();
    rx_msg_mq_.SetMessageType(msg_type_, msg_hdr_desc_);
    tx_msg_mq_.Clear();
    tx_msg_mq_.SetMessageType(msg_type_, msg_hdr_desc_);
  }
  MessageType GetMessageType() const { return msg_type_; }
  const HeaderDescriptionPtr& GetMessageHeaderDescription() const { return msg_hdr_desc_; }

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

  virtual bool OnHandshake();

 private:
  // MSG_NOSIGNAL: Don't generate a SIGPIPE signal if the peer on a stream-oriented socket has closed the connection
  virtual int OnWrite(const void* buf, size_t bytes) { return send(fd_, buf, bytes, MSG_NOSIGNAL); }

  void OnEvents(uint32_t events, void* ctx = nullptr) override;
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

  HeaderDescriptionPtr  msg_hdr_desc_;

  uint32_t      stats_rx_bytes_;
  time_t        stats_rx_last_time_;
  uint32_t      stats_tx_bytes_;
  time_t        stats_tx_last_time_;
};

}  // namespace evt_loop

#endif  // _FD_HANDLER_H

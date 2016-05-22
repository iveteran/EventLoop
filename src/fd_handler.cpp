#include "fd_handler.h"
#include "eventloop.h"
#include <unistd.h>

#define MAX_BYTES_RECEIVE       4096

namespace evt_loop
{

bool ValidFD(int fd) {
  return fd >= 0;
}

IOEvent::IOEvent(int fd, uint32_t events) :
  IEvent(events), fd_(fd)
{
  if (ValidFD(fd_)) {
    EV_Singleton->AddEvent(this);
  }
}
IOEvent::~IOEvent() {
  if (ValidFD(fd_)) {
    EV_Singleton->DeleteEvent(this);
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
void IOEvent::AddReadEvent() {
  if (el_ && !(events_ & IOEvent::READ))
  {
    SetEvents(events_ | IOEvent::READ);
    el_->UpdateEvent(this);
  }
}
void IOEvent::DeleteReadEvent() {
  if (el_ && (events_ & IOEvent::READ))
  {
    SetEvents(events_ & (~IOEvent::READ));
    el_->UpdateEvent(this);
  }
}

void IOEvent::AddWriteEvent() {
  if (el_ && !(events_ & IOEvent::WRITE))
  {
    SetEvents(events_ | IOEvent::WRITE);
    el_->UpdateEvent(this);
  }
}
void IOEvent::DeleteWriteEvent() {
  if (el_ && (events_ & IOEvent::WRITE))
  {
    SetEvents(events_ & (~IOEvent::WRITE));
    el_->UpdateEvent(this);
  }
}

void IOEvent::AddErrorEvent() {
  if (el_ && !(events_ & IOEvent::ERROR))
  {
    SetEvents(events_ | IOEvent::ERROR);
    el_->UpdateEvent(this);
  }
}
void IOEvent::DeleteErrorEvent() {
  if (el_ && (events_ & IOEvent::ERROR))
  {
    SetEvents(events_ & (~IOEvent::ERROR));
    el_->UpdateEvent(this);
  }
}
void IOEvent::ClearAllEvents() {
  if (el_) {
    SetEvents(0);
    el_->UpdateEvent(this);
  }
}

// BufferIOEvent implementation
void BufferIOEvent::ClearBuff() {
  rx_msg_mq_.Clear();
  tx_msg_mq_.Clear();
}
bool BufferIOEvent::TxBuffEmpty() {
  return tx_msg_mq_.Empty();
}

int BufferIOEvent::ReceiveData() {
  char buffer[MAX_BYTES_RECEIVE];
  int read_bytes = std::min(rx_msg_mq_.NeedMore(), (size_t)sizeof(buffer));
  int len = read(fd_, buffer, read_bytes);
  printf("[BufferIOEvent::ReceiveData] fd [%d] to read bytes: %d, got: %d\n", fd_, read_bytes, len);
  if (len < 0) {
    OnError(errno, strerror(errno));
  }
  else if (len == 0 ) {
    OnClosed();
  } else {
    rx_msg_mq_.AppendData(buffer, len);
    MessageMQ::MessageDispatcher processing_msg_cb = std::bind(&BufferIOEvent::OnReceived, this, std::placeholders::_1);
    rx_msg_mq_.Apply(processing_msg_cb);
  }
  return len;
}

int BufferIOEvent::SendData() {
  uint32_t cur_sent = 0;
  while (!tx_msg_mq_.Empty()) {
    const MessagePtr& tx_msg = tx_msg_mq_.First();
    uint32_t tosend = tx_msg->Size();
    int len = write(fd_, tx_msg->Data().data() + sent_, tosend - sent_);
    if (len < 0) {
      OnError(errno, strerror(errno));
      break;
    }
    sent_ += len;
    cur_sent += len;
    if (sent_ == tosend) {
      OnSent(tx_msg.get());
      tx_msg_mq_.EraseFirst();
      sent_ = 0;
    } else {
      /// sent_ less than tosend, breaking the sending loop and wait for next writing event
      break;
    }
  }
  if (tx_msg_mq_.Empty()) {
    DeleteWriteEvent();  // All data in the output buffer has been sent, then remove writing event from epoll
  }
  return cur_sent;
}

void BufferIOEvent::OnEvents(uint32_t events) {
  /// The WRITE events should deal with before the READ events
  if (events & IOEvent::WRITE) {
    SendData();
  }
  if (events & IOEvent::READ) {
    ReceiveData();
  }
  if (events & IOEvent::ERROR) {
    OnError(errno, strerror(errno));
  }
}

void BufferIOEvent::Send(const Message& msg) {
  MessagePtr msg_ptr = CreateMessage(msg);
#ifdef _BINARY_MSG_EXTEND_PACKAGING
  if (msg_type_ == MessageType::BINARY) {
    BinaryMessage* bmsg = static_cast<BinaryMessage*>(msg_ptr.get());
    bmsg->Header()->msg_id = ++msg_seq_;
  }
#endif
  SendInner(msg_ptr);
}

void BufferIOEvent::Send(const string& data, bool bmsg_has_hdr) {
  Send(data.data(), data.size(), bmsg_has_hdr);
}

void BufferIOEvent::SendMore(const string& data) {
  SendMore(data.data(), data.size());
}

void BufferIOEvent::Send(const char *data, uint32_t len, bool bmsg_has_hdr) {
  MessagePtr msg_ptr = CreateMessage(msg_type_, data, len, bmsg_has_hdr);
  if (msg_type_ == MessageType::BINARY) {
    BinaryMessage* bmsg = static_cast<BinaryMessage*>(msg_ptr.get());
#ifdef _BINARY_MSG_EXTEND_PACKAGING
    bmsg->Header()->msg_id = ++msg_seq_;
#endif
    printf("[BufferIOEvent::Send] HDR: %s\n", bmsg->Header()->ToString().c_str());
  }
  printf("[BufferIOEvent::Send] message size: %ld\n", msg_ptr->Size());
  SendInner(msg_ptr);
}

void BufferIOEvent::SendMore(const char *data, uint32_t len) {
  MessagePtr msg_ptr = CreateMessage(msg_type_, data, len, BinaryMessage::HAS_HDR);
  printf("[BufferIOEvent::Send] message size: %ld\n", msg_ptr->Size());
  SendInner(msg_ptr);
}

void BufferIOEvent::SendInner(const MessagePtr& msg) {
  tx_msg_mq_.Push(msg);
  if (!(events_ & IOEvent::WRITE)) {
    AddWriteEvent();  // The output buffer has data now, then add writing event to epoll again if epoll has no writing event
  }
}

}  // namespace evt_loop

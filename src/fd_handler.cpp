#include <errno.h>
#include "fd_handler.h"
#include "eventloop.h"

#define MAX_BYTES_RECEIVE       4096

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
  printf("[IOEvent::~IOEvent] addr: %p, type: %d, fd: %d\n", this, type_, fd_);
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

// BufferIOEvent implementation
void BufferIOEvent::ClearBuff() {
  rx_msg_mq_.Clear();
  tx_msg_mq_.Clear();
}
bool BufferIOEvent::TxBuffEmpty() {
  return tx_msg_mq_.Empty();
}

int BufferIOEvent::ReceiveData(uint32_t& events) {
  char buffer[MAX_BYTES_RECEIVE];
  int total_rx = 0;
  while (!rx_msg_mq_.LastCompletion()) {
    int read_bytes = std::min(rx_msg_mq_.NeedMore(), (size_t)sizeof(buffer));
    if (read_bytes == 0) break;

    int len = OnRead(buffer, read_bytes);
    printf("[BufferIOEvent::ReceiveData] ts: %ld, fd [%d] to read bytes: %d, got: %d\n", Now(), fd_, read_bytes, len);
    if (len < 0) {
      if (errno == EINTR) {
        continue;
      } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      } else {
        events |= FileEvent::ERROR;
        break;
      }
    } else if (len == 0 ) {
      events |= FileEvent::CLOSED;
      break;
    } else {
      rx_msg_mq_.AppendData(buffer, len);
      total_rx += len;

      UpdateRxStats(len);
    }
  }

  if (rx_msg_mq_.FirstCompletion()) {
    MessageMQ::MessageDispatcher processing_msg_cb = std::bind(&BufferIOEvent::OnReceived, this, std::placeholders::_1);
    rx_msg_mq_.Apply(processing_msg_cb);
  }
  return total_rx;
}

int BufferIOEvent::SendData(uint32_t& events) {
  uint32_t cur_sent = 0;
  while (!tx_msg_mq_.Empty()) {
    const MessagePtr& tx_msg = tx_msg_mq_.First();
    uint32_t tosend = tx_msg->Size() - sent_;

    int len = OnWrite(tx_msg->Data().data() + sent_, tosend);
    printf("[BufferIOEvent::SendData] ts: %ld, fd [%d] to send bytes: %d, sent: %d\n", Now(), fd_, tosend, len);
    if (len < 0) {
      if (errno == EINTR) {
        continue;
      } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      } else {
        events |= FileEvent::ERROR;
        break;
      }
    }

    UpdateTxStats(len);

    sent_ += len;
    cur_sent += len;
    if (sent_ == tx_msg->Size()) {
      OnSent(tx_msg.get());
      tx_msg_mq_.EraseFirst();
      sent_ = 0;
    }
  }
  if (tx_msg_mq_.Empty()) {
    DeleteWriteEvent();  // All data in the output buffer has been sent, then remove writing event from epoll
    if (close_wait_)
      events |= FileEvent::CLOSED;
  }
  return cur_sent;
}

void BufferIOEvent::OnEvents(uint32_t events) {
  bool success = false;
  if ((events & FileEvent::WRITE || events & FileEvent::READ) &&
          (state_ == CONNECTED || state_ == HANDSHAKING)) {
    success = OnHandshake();
    if (!success) events |= FileEvent::CLOSED;
  } else {
    /// The WRITE events should deal with before the READ events
    if (events & FileEvent::WRITE) {
        SendData(events);
    }
    if (events & FileEvent::READ) {
        ReceiveData(events);
    }
  }

  if (events & FileEvent::CLOSED) {
    OnClosed();
  } else if ((events & FileEvent::ERROR)) {
    OnError(errno, strerror(errno));
  }
}

bool BufferIOEvent::Send(const Message& msg) {
  MessagePtr msg_ptr = CreateMessage(msg);
  if (msg_ptr) {
#ifdef _BINARY_MSG_EXTEND_PACKAGING
    if (msg_type_ == MessageType::BINARY) {
      BinaryMessage* bmsg = static_cast<BinaryMessage*>(msg_ptr.get());
      bmsg->Header()->msg_id = ++msg_seq_;
    }
#endif
    return SendInner(msg_ptr);
  } else {
    printf("[BufferIOEvent::Send] Create message failed");
    return false;
  }
}

bool BufferIOEvent::Send(const string& data, bool bmsg_has_hdr) {
  return Send(data.data(), data.size(), bmsg_has_hdr);
}

bool BufferIOEvent::SendMore(const string& data) {
  return SendMore(data.data(), data.size());
}

bool BufferIOEvent::Send(const char *data, uint32_t len, bool bmsg_has_hdr) {
  MessagePtr msg_ptr = CreateMessage(msg_type_, data, len, bmsg_has_hdr);
  if (msg_ptr) {
    if (msg_type_ == MessageType::BINARY) {
      BinaryMessage* bmsg = static_cast<BinaryMessage*>(msg_ptr.get());
#ifdef _BINARY_MSG_EXTEND_PACKAGING
      bmsg->Header()->msg_id = ++msg_seq_;
#endif
      printf("[BufferIOEvent::Send] HDR: %s\n", bmsg->Header()->ToString().c_str());
    }
    printf("[BufferIOEvent::Send] message size: %ld\n", msg_ptr->Size());
    return SendInner(msg_ptr);
  } else {
    printf("[BufferIOEvent::Send] Create message failed");
    return false;
  }
}

bool BufferIOEvent::SendMore(const char *data, uint32_t len) {
  MessagePtr msg_ptr = CreateMessage(msg_type_, data, len, BinaryMessage::HAS_HDR);
  if (msg_ptr) {
    printf("[BufferIOEvent::SendMore] message size: %ld\n", msg_ptr->Size());
    return SendInner(msg_ptr);
  } else {
    printf("[BufferIOEvent::SendMore] Create message failed");
    return false;
  }
}

bool BufferIOEvent::SendInner(const MessagePtr& msg) {
  if (FD() < 0) return false;
  tx_msg_mq_.Push(msg);
  if (!(events_ & FileEvent::WRITE)) {
    AddWriteEvent();  // The output buffer has data now, then add writing event to epoll again if epoll has no writing event
  }
  return true;
}

void BufferIOEvent::UpdateRxStats(uint32_t rx_bytes) {
  stats_rx_bytes_ += rx_bytes;
  stats_rx_last_time_ = Now();
}
void BufferIOEvent::UpdateTxStats(uint32_t tx_bytes) {
  stats_tx_bytes_ += tx_bytes;
  stats_tx_last_time_ = Now();
}

}  // namespace evt_loop

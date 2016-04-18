#include "message.h"

namespace evt_loop {

const char* CRLFMessage::TERMINAL_LABEL = "\r\n";

size_t CRLFMessage::AppendData(const char* data, uint32_t size) {
  if (data == NULL || size == 0 || Completion())
    return 0;
  // FIXME: should processing the case that "\r\n" was splitted in two data
  size_t feed_size = size;
  char* tl_ptr = (char*)strstr(data, TERMINAL_LABEL);
  if (tl_ptr != NULL) {
    tl_ptr += 2;
    feed_size = tl_ptr - data;
  }
  data_.append(data, feed_size);
  return feed_size;
}
size_t CRLFMessage::AssignData(const char* data, uint32_t size, bool has_hdr) {
  UNUSED(has_hdr);
  data_.clear();
  return AppendData(data, size);
}

size_t JsonMessage::AppendData(const char* data, uint32_t size) {
  if (data == NULL || size == 0 || Completion())
    return 0;
  size_t feed_size = size;
  for (uint32_t i = 0; i < size; i++) {
    if (data[i] == '{') {
      lbc_++;
    } else if (data[i] == '}') {
      rbc_++;
    }
    if (lbc_ == rbc_) {
      feed_size = i + 1;
      break;
    }
  }
  data_.append(data, feed_size);
  return feed_size;
}
size_t JsonMessage::AssignData(const char* data, uint32_t size, bool has_hdr) {
  UNUSED(has_hdr);
  data_.clear();
  lbc_ = rbc_ = 0;
  return AppendData(data, size);
}
  
BinaryMessage::BinaryMessage(const std::string& data, bool has_hdr) : Message(MessageType::BINARY) {
  AssignData(data.data(), data.size(), has_hdr);
  ResetHeader();
}
BinaryMessage::BinaryMessage(const char* data, uint32_t length, bool has_hdr) : Message(MessageType::BINARY) {
  AssignData(data, length, has_hdr);
  ResetHeader();
}
BinaryMessage::BinaryMessage(const BinaryMessage& other) : Message(MessageType::BINARY) {
  data_ = other.data_;
  ResetHeader();
}
BinaryMessage& BinaryMessage::operator=(const BinaryMessage& rvalue) {
  data_ = rvalue.data_;
  ResetHeader();
  return *this;
}

size_t BinaryMessage::AppendData(const char* data, uint32_t length) {
  if (data == NULL || length == 0) return 0;

  data_.append(data, length);

  if (hdr_ == NULL && data_.size() >= sizeof(HDR)) {
    hdr_ = (HDR*)data_.data();
#ifndef _BINARY_MSG_MINIMUM_PACKAGING
    printf("[BinaryMessage::AppendData] HDR{ length: %d, msg_id: %d }\n", hdr_->length, hdr_->msg_id);
#else
    printf("[BinaryMessage::AppendData] HDR{ length: %d }\n", hdr_->length);
#endif
    if (data_.capacity() < hdr_->length) {
      data_.reserve(hdr_->length);
      hdr_ = (HDR*)data_.data();
    }
  }
  return length;   // FIXME: the return value maybe less than length
}

size_t BinaryMessage::AssignData(const char* data, uint32_t length, bool has_hdr) {
  if (data == NULL || length == 0) return 0;

  if (!has_hdr) {
    HDR msg_hdr;
    msg_hdr.length = length + sizeof(msg_hdr);
    data_.reserve(msg_hdr.length);
    data_.append((char*)&msg_hdr, sizeof(msg_hdr));
  }
  data_.append(data, length);

  return length;   // FIXME: the return value maybe less than length
}

void BinaryMessage::ResetHeader() {
  if (data_.size() >= sizeof(HDR)) {
    hdr_ = (HDR*)data_.data();
  }
}

size_t BinaryMessage::Payload(std::string& payload) const {
  size_t pl_length = hdr_->length - sizeof(HDR);
  payload.assign((char*)(hdr_->payload), pl_length);
  return pl_length;
}

size_t BinaryMessage::MoreSize() const {
  size_t more_size = 0;
  if (data_.size() < sizeof(HDR)) {
    more_size = sizeof(HDR) - data_.size();
  } else {
    more_size = hdr_->length - data_.size();
  }
  return more_size;
}

MessagePtr CreateMessage(MessageType msg_type) {
  MessagePtr msg_ptr;
  switch (msg_type) {
    case MessageType::CRLF:
      msg_ptr = std::make_shared<CRLFMessage>();
      break;
    case MessageType::JSON:
      msg_ptr = std::make_shared<JsonMessage>();
      break;
    case MessageType::BINARY:
      msg_ptr = std::make_shared<BinaryMessage>();
      break;
    default:
      break;
  }
  return msg_ptr;
}

MessagePtr CreateMessage(MessageType msg_type, const char* data, size_t length, bool bmsg_has_no_hdr) {
  MessagePtr msg_ptr;
  switch (msg_type) {
    case MessageType::CRLF:
      msg_ptr = std::make_shared<CRLFMessage>(data, length);
      break;
    case MessageType::JSON:
      msg_ptr = std::make_shared<JsonMessage>(data, length);
      break;
    case MessageType::BINARY:
      msg_ptr = std::make_shared<BinaryMessage>(data, length, bmsg_has_no_hdr);
      break;
    default:
      break;
  }
  return msg_ptr;
}

MessagePtr CreateMessage(const Message& msg) {
  MessagePtr msg_ptr;
  switch (msg.Type()) {
    case MessageType::CRLF:
      msg_ptr = std::make_shared<CRLFMessage>(dynamic_cast<const CRLFMessage&>(msg));
      break;
    case MessageType::JSON:
      msg_ptr = std::make_shared<JsonMessage>(dynamic_cast<const JsonMessage&>(msg));
      break;
    case MessageType::BINARY:
      msg_ptr = std::make_shared<BinaryMessage>(dynamic_cast<const BinaryMessage&>(msg));
      break;
    default:
      break;
  }
  return msg_ptr;
}

MessagePtr& MessageMQ::Last() {
  if (mq_.empty()) {
    mq_.push(CreateMessage(msg_type_));
  }
  return mq_.back();
}
MessagePtr& MessageMQ::First() {
  if (mq_.empty()) {
    mq_.push(CreateMessage(msg_type_));
  }
  return mq_.front();
}
void MessageMQ::AppendData(const char* data, uint32_t size) {
  size_t feeds = 0;
  while (feeds < size) {
    if (Last()->Completion()) {
      mq_.push(CreateMessage(msg_type_));
    }
    feeds += Last()->AppendData(&data[feeds], size - feeds);
    if (Last()->Completion()) {
      printf("[MessageMQ] Recieved a complation message, type: %d, size: %u\n", Last()->Type(), Last()->Size());
    }
  }
}
void MessageMQ::Apply(MessageDispatcher& cb) {
  while (!mq_.empty() && mq_.front()->Completion()) {
    cb(mq_.front().get());
    mq_.pop();
  }
}

}  // evt_loop

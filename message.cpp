#include "message.h"

namespace evt_loop {

Message::Message(const std::string& data, bool has_hdr) {
  AssignData(data.data(), data.size(), has_hdr);
  ResetHeader();
}
Message::Message(const char* data, uint32_t length, bool has_hdr) {
  AssignData(data, length, has_hdr);
  ResetHeader();
}
Message::Message(const Message& other) {
  data_ = other.data_;
  ResetHeader();
}
Message& Message::operator=(const Message& rvalue) {
  data_ = rvalue.data_;
  ResetHeader();
  return *this;
}
Message& Message::operator=(const std::string& rvalue) {
  /// the rvalue MUST has message header
  data_ = rvalue;
  ResetHeader();
  return *this;
}

void Message::AppendData(const char* data, uint32_t length) {
  if (data == NULL || length == 0) return;

  data_.append(data, length);

  if (hdr_ == NULL && data_.size() >= sizeof(HDR)) {
    hdr_ = (HDR*)data_.data();
    printf("[Message::AppendData] HDR{ length: %d, msg_id: %d }\n", hdr_->length, hdr_->msg_id);
    if (data_.capacity() < hdr_->length) {
      data_.reserve(hdr_->length);
      hdr_ = (HDR*)data_.data();
    }
  }
}
void Message::ResetHeader() {
  if (data_.size() >= sizeof(HDR)) {
    hdr_ = (HDR*)data_.data();
  }
}

size_t Message::Payload(std::string& payload) const {
  size_t pl_length = hdr_->length - sizeof(HDR);
  payload.assign((char*)(hdr_->payload), pl_length);
  return pl_length;
}

size_t Message::MoreSize() const {
  size_t more_size = 0;
  if (data_.size() < sizeof(HDR)) {
    more_size = sizeof(HDR) - data_.size();
  } else {
    more_size = hdr_->length - data_.size();
  }
  return more_size;
}

void Message::AssignData(const char* data, uint32_t length, bool has_hdr) {
  if (data == NULL || length == 0) return;

  if (!has_hdr) {
    HDR msg_hdr;
    msg_hdr.length = length + sizeof(msg_hdr);
    data_.reserve(msg_hdr.length);
    data_.append((char*)&msg_hdr, sizeof(msg_hdr));
  }
  data_.append(data, length);
}

}  // evt_loop

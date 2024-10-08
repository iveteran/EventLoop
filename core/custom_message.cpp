#include <arpa/inet.h>
#include "custom_message.h"
#include "logger.h"

namespace evt_loop {

CustomMessage::CustomMessage(
    const HeaderDescriptionPtr& hdr_desc, const std::string& data
    ) : Message(MessageType::CUSTOM, hdr_desc) {
  AssignData(data.data(), data.size());
  ResetHeader();
}

CustomMessage::CustomMessage(
    const HeaderDescriptionPtr& hdr_desc, const char* data, uint32_t length
    ) : Message(MessageType::CUSTOM, hdr_desc) {
  AssignData(data, length);
  ResetHeader();
}

CustomMessage::CustomMessage(const CustomMessage& other) : Message(MessageType::CUSTOM) {
  hdr_desc_ = other.hdr_desc_;
  data_ = other.data_;
  ResetHeader();
}

CustomMessage& CustomMessage::operator=(const CustomMessage& rvalue) {
  hdr_desc_ = rvalue.hdr_desc_;
  data_ = rvalue.data_;
  ResetHeader();
  return *this;
}

size_t CustomMessage::AppendData(const char* data, uint32_t length) {
  if (data == NULL || length == 0) return 0;

  data_.append(data, length);

  if (data_.size() >= HeaderSize()) {
    DecodeHeader();
    string hex_data = DumpHex(hdr_desc_->hdr_len);
    el_logger->debug("[CustomMessage::AppendData] message header bytes: {}", hex_data);
    el_logger->debug("[CustomMessage::AppendData] HDR: {}", hdr_desc_->ToString());
    if (data_.capacity() < MessageSize()) {
      data_.reserve(MessageSize());
      DecodeHeader();
    }
  }
  return length;   // FIXME: the return value maybe less than length
}

size_t CustomMessage::AssignData(const char* data, uint32_t length, bool has_hdr) {
  UNUSED(has_hdr);
  if (data == NULL || length == 0) return 0;

  data_.append(data, length);

  return length;   // FIXME: the return value maybe less than length
}

void CustomMessage::ResetHeader() {
  if (data_.size() >= HeaderSize()) {
    DecodeHeader();
  }
}

void CustomMessage::DecodeHeader() {
  hdr_desc_->hdr_ptr = data_.data();
  hdr_desc_->payload_ptr = hdr_desc_->hdr_ptr + hdr_desc_->hdr_len;
  const char* payload_len_ptr = hdr_desc_->hdr_ptr + hdr_desc_->payload_len_offset;
  switch (hdr_desc_->payload_len_bytes) {
    case sizeof(uint8_t):
      hdr_desc_->payload_len = *(uint8_t*)payload_len_ptr;
      break;
    case sizeof(uint16_t):
      hdr_desc_->payload_len = ntohs(*(uint16_t*)payload_len_ptr);
      break;
    case sizeof(uint32_t):
      hdr_desc_->payload_len = ntohl(*(uint32_t*)payload_len_ptr);
      break;
    //case sizeof(uint64_t):
    //  hdr_desc_->payload_len = ntohll(*(uint64_t*)payload_len_ptr);
    //  break;
    default:
      el_logger->critical("[CustomMessage::DecodeHeader] Unsupported payload length bytes: {}", hdr_desc_->payload_len_bytes);
      abort();
      break;
  }
}

size_t CustomMessage::Payload(std::string& payload) const {
  int payload_len;
  if (hdr_desc_->is_payload_len_including_self)
    payload_len = hdr_desc_->payload_len - hdr_desc_->payload_len_bytes;
  else
    payload_len = hdr_desc_->payload_len;
  payload.assign((char*)(hdr_desc_->payload_ptr), payload_len);
  return payload_len;
}

size_t CustomMessage::MoreSize() const {
  size_t more_size = 0;
  size_t data_size = data_.size();
  if (data_size < HeaderSize()) {
    more_size = HeaderSize() - data_size;
  } else {
    more_size = MessageSize() - data_size;
  }
  return more_size;
}

}  // evt_loop

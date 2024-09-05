#ifndef _CUSTOM_MESSAGE_H
#define _CUSTOM_MESSAGE_H

#include "message.h"

namespace evt_loop {

class CustomMessage : public Message {
  public:
  CustomMessage(const HeaderDescriptionPtr& hdr_desc) : Message(MessageType::CUSTOM, hdr_desc) { }
  CustomMessage(const HeaderDescriptionPtr& hdr_desc, const std::string& data);
  CustomMessage(const HeaderDescriptionPtr& hdr_desc, const char* data, uint32_t length);
  CustomMessage(const CustomMessage& other);
  CustomMessage& operator=(const CustomMessage& rvalue);

  size_t AppendData(const char* data, uint32_t length);
  size_t AssignData(const char* data, uint32_t length, bool has_hdr = true);

  void ResetHeader();
  void DecodeHeader();
  size_t Payload(std::string& payload) const;
  size_t MoreSize() const;

  size_t MessageSize() const {
      return hdr_desc_ ? hdr_desc_->hdr_len + hdr_desc_->payload_len -
      (hdr_desc_->is_payload_len_including_self ? hdr_desc_->payload_len_bytes : 0) : 0;
  }
  size_t HeaderSize() const       { return hdr_desc_ ? hdr_desc_->hdr_len : 0; }
  const char* Payload() const     { return hdr_desc_ ? hdr_desc_->payload_ptr : NULL; }
  size_t PayloadSize() const      { return hdr_desc_ ? hdr_desc_->payload_len : 0; }
  bool Completion() const         { return hdr_desc_ && MessageSize() == data_.size(); }
  void Clear()                    { Message::Clear(); hdr_desc_ = nullptr; }
};

}  // evt_loop
#endif // _CUSTOM_MESSAGE_H

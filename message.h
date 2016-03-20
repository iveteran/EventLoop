#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <string.h>
#include <deque>
#include <memory>

namespace evt_loop {

class Message {
  public:
#pragma pack(1)
  struct HDR {
    uint32_t  length;
#ifndef _MSG_MINIMUM_PACKAGING
    uint16_t  msg_type;
    uint32_t  msg_id;
    uint8_t   protocol;
#endif
    char      payload[0];
    HDR()     { memset(this, 0, sizeof(*this)); }
  };
#pragma pack()

  enum { HAS_NO_HDR = false, HAS_HDR = true };

  public:
  Message() : hdr_(NULL) { }
  Message(const std::string& data, bool has_hdr = HAS_HDR);
  Message(const char* data, uint32_t length, bool has_hdr = HAS_HDR);
  Message(const Message& other);

  Message& operator=(const Message& rvalue);
  Message& operator=(const std::string& rvalue); /// the rvalue MUST has message header

  void AppendData(const char* data, uint32_t length);
  void ResetHeader();
  size_t Payload(std::string& payload) const;
  size_t MoreSize() const;

  Message::HDR* Header() const    { return hdr_; }
  const std::string& Data() const { return data_; }
  const char* Payload() const     { return (char*)(hdr_->payload); }
  size_t PayloadSize() const      { return hdr_ == NULL ? 0 : hdr_->length - sizeof(HDR); }
  size_t Size() const             { return data_.size(); }
  bool Empty() const              { return data_.empty(); }
  bool Completion() const         { return hdr_ && hdr_->length == data_.size(); }
  void Clear()                    { hdr_ = NULL; data_.clear(); }

  private:
  void AssignData(const char* data, uint32_t length, bool has_hdr = HAS_HDR);

  private:
  HDR*          hdr_;
  std::string   data_;
};

typedef std::shared_ptr<Message>  MessagePtr;
typedef std::deque<MessagePtr>    MessageMQ;

}  // evt_loop
#endif  // _MESSAGE_H

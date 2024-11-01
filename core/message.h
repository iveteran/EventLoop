#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <string.h>
#include <string>
#include <queue>
#include <memory>
#include <functional>

#define UNUSED(var) ((void)var)

namespace evt_loop {

enum MessageType {
  UNKNOWN,
  BINARY,
  CRLF,
  JSON,
  TLV,  // Tag, Length, Value
  CUSTOM,
};

struct HeaderDescription {
  // pre-defined fields
  uint32_t          hdr_len = 0;
  uint32_t          payload_len_offset = 0;
  uint32_t          payload_len_bytes = 0;
  bool              is_payload_len_including_self = false;
  std::string       heartbeat_request;
  std::string       heartbeat_response;

  // be decode fields
  const char*       hdr_ptr = nullptr;
  const char*       payload_ptr = nullptr;
  uint32_t          payload_len = 0;

  std::string ToString() const {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "{ hdr_len: %u, payload_len_offset: %u, payload_len_bytes: %u, "
            "is_payload_len_including_self: %u, hdr_ptr: 0x%p, payload_ptr: 0x%p, payload_len: %u }",
        hdr_len, payload_len_offset, payload_len_bytes,
        is_payload_len_including_self, hdr_ptr, payload_ptr, payload_len);
    return buffer;
  }
};
typedef std::shared_ptr<HeaderDescription> HeaderDescriptionPtr;

class Message {
  public:
  Message(MessageType type, const HeaderDescriptionPtr& hdr_desc = nullptr) : type_(type), hdr_desc_(hdr_desc) { }

  void SetHeaderDescription(const HeaderDescriptionPtr& hdr_desc) {
    hdr_desc_ = hdr_desc;
  }
  const HeaderDescriptionPtr& GetHeaderDescription() const {
    return hdr_desc_;
  }

  virtual size_t MoreSize() const = 0;
  virtual bool Completion() const = 0;
  virtual size_t AppendData(const char* data, uint32_t length) = 0;
  virtual size_t AssignData(const char* data, uint32_t length, bool has_hdr = false) = 0;

  MessageType Type() const        { return type_; }
  const std::string& Data() const { return data_; }
  size_t Size() const             { return data_.size(); }
  bool Empty() const              { return data_.empty(); }

  HeaderDescriptionPtr HeaderDescription() const { return hdr_desc_; }

  std::string DumpHex(size_t max_bytes = 0) const;
  std::string DumpHexWithChars(size_t max_bytes = 0) const;

  virtual void Clear()                    { data_.clear(); }
  virtual const char* Payload() const     { return data_.data(); }
  virtual size_t PayloadSize() const      { return data_.size(); }

  protected:
  MessageType   type_;
  std::string   data_;
  HeaderDescriptionPtr  hdr_desc_;
};
typedef std::shared_ptr<Message>  MessagePtr;

class CRLFMessage : public Message {
  public:
  CRLFMessage() : Message(MessageType::CRLF) { }

  CRLFMessage(const std::string& data) : Message(MessageType::CRLF) {
    AssignData(data.data(), data.size());
  }
  CRLFMessage(const char* data, uint32_t length) : Message(MessageType::CRLF) {
    AssignData(data, length);
  }
  CRLFMessage(const CRLFMessage& other) : Message(MessageType::CRLF) {
    data_ = other.data_;
  }
  CRLFMessage& operator=(const CRLFMessage& rvalue) {
    data_ = rvalue.data_;
    return *this;
  }

  size_t MoreSize() const { return 1024; }
  bool Completion() const { return !memcmp((char*)&data_[data_.size() - 2], TERMINAL_LABEL, 2); }
  size_t AppendData(const char* data, uint32_t size);
  size_t AssignData(const char* data, uint32_t size, bool has_hdr = false);

  private:
  static const char* TERMINAL_LABEL;
};

class JsonMessage : public Message {
  public:
  JsonMessage() : Message(MessageType::JSON), lbc_(0), rbc_(0) { }

  JsonMessage(const std::string& data) : Message(MessageType::JSON) {
    AssignData(data.data(), data.size());
  }
  JsonMessage(const char* data, uint32_t length) : Message(MessageType::JSON) {
    AssignData(data, length);
  }
  JsonMessage(const JsonMessage& other) : Message(MessageType::JSON) {
    data_ = other.data_;
  }
  JsonMessage& operator=(const JsonMessage& rvalue) {
    data_ = rvalue.data_;
    return *this;
  }

  size_t MoreSize() const { return 4096; }
  bool Completion() const { return lbc_ != 0 && lbc_ == rbc_; }
  size_t AppendData(const char* data, uint32_t size);
  size_t AssignData(const char* data, uint32_t size, bool has_hdr = false);
  
  private:
  size_t lbc_;
  size_t rbc_;;
};

class BinaryMessage : public Message {
  public:
#pragma pack(1)
  struct HDR {
    uint32_t  length;
#ifdef _BINARY_MSG_EXTEND_PACKAGING
    uint16_t  msg_type;
    uint32_t  msg_id;
    uint8_t   protocol;
#endif
    char      payload[0];
    HDR()     { memset(this, 0, sizeof(*this)); }
    std::string ToString() const {
      char buffer[128];
#ifdef _BINARY_MSG_EXTEND_PACKAGING
      snprintf(buffer, sizeof(buffer), "{ length: %u, msg_type: %u, msg_id: %u, protocol: %u }",
          length, msg_type, msg_id, protocol);
#else
      snprintf(buffer, sizeof(buffer), "{ length: %u }", length);
#endif
      return buffer;
    }
  };
#pragma pack()

  enum { HAS_NO_HDR = false, HAS_HDR = true };

  public:
  BinaryMessage() : Message(MessageType::BINARY), hdr_(NULL) { }
  BinaryMessage(const std::string& data, bool has_hdr = HAS_HDR);
  BinaryMessage(const char* data, uint32_t length, bool has_hdr = HAS_HDR);
  BinaryMessage(const BinaryMessage& other);
  BinaryMessage& operator=(const BinaryMessage& rvalue);

  size_t AppendData(const char* data, uint32_t length);
  size_t AssignData(const char* data, uint32_t length, bool has_hdr = HAS_HDR);

  void ResetHeader();
  size_t Payload(std::string& payload) const;
  size_t MoreSize() const;

  BinaryMessage::HDR* Header() const    { return hdr_; }
  const char* Payload() const     { return hdr_ ? (const char*)(hdr_->payload) : NULL; }
  size_t PayloadSize() const      { return hdr_ ? hdr_->length - sizeof(HDR) : 0; }
  bool Completion() const         { return hdr_ && hdr_->length == data_.size(); }
  void Clear()                    { Message::Clear(); hdr_ = NULL; }

  private:
  HDR*          hdr_;
};

MessagePtr CreateMessage(MessageType msg_type, const HeaderDescriptionPtr& hdr_desc = nullptr);
MessagePtr CreateMessage(MessageType msg_type, const char* data, size_t length,
        bool bmsg_has_no_hdr = BinaryMessage::HAS_NO_HDR, const HeaderDescriptionPtr& hdr_desc = nullptr);
MessagePtr CreateMessage(const Message& msg);

class MessageMQ {
  public:
  typedef std::function<void (const Message*) > MessageDispatcher;

  void SetMessageType(const MessageType& msg_type, const HeaderDescriptionPtr& msg_hdr_desc) {
      msg_type_ = msg_type;
      msg_hdr_desc_ = msg_hdr_desc;
  }
  size_t Size() const { return mq_.size(); }
  bool Empty() const { return mq_.empty(); }
  void Clear() { while (!mq_.empty()) { mq_.pop(); } }
  MessagePtr& Last();
  void Push(const MessagePtr& msg) { mq_.push(msg); }
  MessagePtr& First();
  void EraseFirst() { mq_.pop(); }

  size_t NeedMore() { return Last()->MoreSize(); }
  bool LastCompletion() { return Last()->Completion(); }
  bool FirstCompletion() { return First()->Completion(); }
  bool HasCompletion() { return First()->Completion(); }

  void AppendData(const char* data, uint32_t size);
  void Apply(MessageDispatcher& cb);

  private:
  MessageType msg_type_;
  HeaderDescriptionPtr msg_hdr_desc_;
  std::queue<MessagePtr> mq_;
};

}  // evt_loop
#endif  // _MESSAGE_H

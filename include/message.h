#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <string.h>
#include <queue>
#include <memory>

#define UNUSED(var) ((void)var)

namespace evt_loop {

enum MessageType {
  UNKNOWN,
  BINARY,
  CRLF,
  JSON,
  TLV,
};

class Message {
  public:
  Message(MessageType type) : type_(type) { }

  virtual size_t MoreSize() const = 0;
  virtual bool Completion() const = 0;
  virtual size_t AppendData(const char* data, uint32_t length) = 0;
  virtual size_t AssignData(const char* data, uint32_t length, bool has_hdr = false) = 0;

  MessageType Type() const        { return type_; }
  const std::string& Data() const { return data_; }
  size_t Size() const             { return data_.size(); }
  bool Empty() const              { return data_.empty(); }

  virtual void Clear()                    { data_.clear(); }
  virtual const char* Payload() const     { return data_.data(); }
  virtual size_t PayloadSize() const      { return data_.size(); }

  protected:
  MessageType   type_;
  std::string   data_;
};

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
  const char* Payload() const     { return (char*)(hdr_->payload); }
  size_t PayloadSize() const      { return hdr_ == NULL ? 0 : hdr_->length - sizeof(HDR); }
  bool Completion() const         { return hdr_ && hdr_->length == data_.size(); }
  void Clear()                    { Message::Clear(); hdr_ = NULL; }

  private:
  HDR*          hdr_;
};

typedef std::shared_ptr<Message>  MessagePtr;

MessagePtr CreateMessage(MessageType msg_type);
MessagePtr CreateMessage(MessageType msg_type, const char* data, size_t length, bool bmsg_has_no_hdr = BinaryMessage::HAS_NO_HDR);
MessagePtr CreateMessage(const Message& msg);

class MessageMQ {
  public:
  typedef std::function<void (const Message*) > MessageDispatcher;

  void SetMessageType(const MessageType& msg_type) { msg_type_ = msg_type; }
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

  void AppendData(const char* data, uint32_t size);
  void Apply(MessageDispatcher& cb);

  private:
  MessageType msg_type_;
  std::queue<MessagePtr> mq_;
};

}  // evt_loop
#endif  // _MESSAGE_H

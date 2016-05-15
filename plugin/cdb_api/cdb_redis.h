#ifndef _CDB_REDIS_H
#define _CDB_REDIS_H

#include <async.h>
#include <hiredis.h>
#include "cdb_client.h"

namespace cdb_api {

class RedisMessage : public CDBMessage {
  public:
  RedisMessage(const redisReply* reply = NULL) : redis_reply_(reply) { }
  //~RedisMessage() { ReleaseReplyObject(); }
  RedisMessage& operator=(const redisReply* reply) {
    ReleaseReplyObject();
    redis_reply_ = reply;
    return *this;
  }
  void ReleaseReplyObject() {
    freeReplyObject((void*)redis_reply_);
    redis_reply_ = NULL;
  }
  void SetReply(const void* reply) {
    ReleaseReplyObject();
    redis_reply_ = (const redisReply*)reply;
  }
  const void* GetReply() const {
    return redis_reply_;
  }

  private:
  const redisReply* redis_reply_;
};

class RedisAsyncClient : public CDBClient, public IOEvent {
  public:
  RedisAsyncClient() : redis_ctx_(NULL) { }
  bool Init(const char* host, uint16_t port, const CDBCallbacksPtr& cdb_cbs = nullptr, bool auto_reconnect = true);

  bool IsReady();
  bool Connect();
  void Disconnect();

  void SetCallbacks(const CDBCallbacksPtr& cdb_cbs);
  redisAsyncContext* RedisContext() { return redis_ctx_; }

  bool SendCommand(CDBMessage* reply_msg, const char* format, ...);
  bool SendCommand(const OnReplyCallback& reply_cb, const char* format, ...);

  void OnRedisReply(const redisAsyncContext* ctx, redisReply* reply);
  void OnRedisConnect(const redisAsyncContext* ctx, int status);
  void OnRedisDisconnect(const redisAsyncContext* ctx, int status);

  private:
  int SetContext(redisAsyncContext * ctx);
  bool Connect_(bool reconnect = false);

  void OnEvents(uint32_t events);
  void OnError(int errcode, const char* errstr);
  void DefaultOnReplyCb(CDBClient* cdbc, const CDBMessage* cdb_msg);

  private:
  redisAsyncContext*  redis_ctx_;
  CDBCallbacksPtr     cdb_cbs_;
  CDBReplyCallbackQueue   reply_cb_queue_;
};

class RedisClient : public CDBClient {
  public:
  RedisClient() : redis_ctx_(NULL) { }
  ~RedisClient() { Disconnect(); }

  bool IsReady();
  bool Connect();
  void Disconnect();

  bool SendCommand(CDBMessage* reply_msg, const char* format, ...);
  bool SendCommand(const OnReplyCallback& reply_cb, const char* format, ...);

  private:
  bool Connect_(bool reconnect = false);

  private:
  redisContext*   redis_ctx_;
};

}  // namespace cdb_api

#endif  // _CDB_REDIS_H

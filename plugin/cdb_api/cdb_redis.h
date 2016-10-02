#ifndef _CDB_REDIS_H
#define _CDB_REDIS_H

#include <async.h>
#include <hiredis.h>
#include "cdb_client.h"

namespace cdb_api {

class RedisReply : public CDBReply {
  public:
  RedisReply(const redisReply* reply = NULL) : redis_reply_(reply) { }
  //~RedisReply() { ReleaseReplyObject(); }
  RedisReply& operator=(const redisReply* reply) {
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
  RedisAsyncClient(bool auto_reconnect = true) : CDBClient(auto_reconnect), redis_ctx_(NULL), connected_(false) { }
  ~RedisAsyncClient();
  bool Init(const char* host, uint16_t port, const CDBCallbacksPtr& cdb_cbs = nullptr, bool auto_reconnect = true);

  bool IsReady() const;
  bool Connected() const;
  void Disconnect();
  void HandleConnect();
  void HandleDisconnect();

  void SetCallbacks(const CDBCallbacksPtr& cdb_cbs);
  redisAsyncContext* RedisContext() { return redis_ctx_; }

  bool SendCommand(CDBReply* reply_msg, const char* format, ...);
  bool SendCommand(const OnReplyCallback& reply_cb, const char* format, ...);

  void OnRedisReply(const redisAsyncContext* ctx, redisReply* reply);
  void OnRedisConnect(const redisAsyncContext* ctx, int status);
  void OnRedisDisconnect(const redisAsyncContext* ctx, int status);

  bool HasError() const;
  const char* GetLastError() const;

  private:
  bool SetRedisCallbacks();
  bool Connect_(bool reconnect = false);

  void OnEvents(uint32_t events);
  void OnError(CDBClient* cdbclient, int errcode, const char* errstr);
  void DefaultOnReplyCb(CDBClient* cdbc, const CDBReply* cdb_msg);

  private:
  redisAsyncContext*  redis_ctx_;
  bool connected_;
  //CDBCallbacksPtr     cdb_cbs_;
  CDBReplyCallbackQueue   reply_cb_queue_;
  char m_errstr[256];
};

class RedisClient : public CDBClient {
  public:
  RedisClient(bool auto_reconnect = true) : CDBClient(auto_reconnect), redis_ctx_(NULL) { }
  ~RedisClient() { Disconnect(); }

  bool IsReady() const;
  void Disconnect();

  bool SendCommand(CDBReply* reply_msg, const char* format, ...);
  bool SendCommand(const OnReplyCallback& reply_cb, const char* format, ...);

  bool HasError() const;
  const char* GetLastError() const;

  private:
  bool Connect_(bool reconnect = false);

  private:
  redisContext*   redis_ctx_;
  char m_errstr[256];
};

}  // namespace cdb_api

#endif  // _CDB_REDIS_H

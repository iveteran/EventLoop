#ifndef _CDB_REDIS_H
#define _CDB_REDIS_H

#include <async.h>
#include <hiredis.h>
#include "cdb_client.h"

#define ERR_CONNECTION_NOT_READY    "RedisClient: client is not connected"

namespace cdb_api {

string GetRedisKeyFromCommand(char* cmd_str);
string GetRedisKeyFromCommand(const char* format, va_list ap, string* fmt_cmd = NULL);

class RedisClient;
class RedisAsyncClient;
class RedisSyncClient;
struct RedisRequest;

typedef std::shared_ptr<RedisClient>        RedisClientPtr;
typedef std::shared_ptr<RedisAsyncClient>   RedisAsyncClientPtr;
typedef std::shared_ptr<RedisSyncClient>    RedisSyncClientPtr;
typedef std::shared_ptr<RedisRequest>       RedisRequestPtr;
typedef std::queue<RedisRequestPtr>         RedisRequestQueue;
typedef std::function<void (RedisClient*, const RedisRequestPtr&, const string&, uint16_t)>  RedisAskReplyCallback;
typedef std::function<void (RedisClient*, const RedisRequestPtr&, const string&, uint16_t)>  RedisRedirectReplyCallback;
typedef std::function<void (RedisClient*, const RedisRequestPtr&)>  RedisClusterDownReplyCallback;

enum ReplyState { UNDEFINED, ASK, MOVED, CLUSTERDOWN, FAILED, NORMAL };

struct RedisRequest
{
  enum Step { UNDEFINED, ASK, REDIRECT, RETRY, FAILED, FINISH };

  RedisRequest(const OnReplyCallback& reply_cb, const char* format, va_list ap)
  {
    reply_cb_ = reply_cb;

    va_list ap_copy1;
    va_copy(ap_copy1, ap);
    key_ = GetRedisKeyFromCommand(format, ap_copy1, &fmt_cmd_);
    va_end(ap_copy1);

    char* cmd_buf = NULL;
    va_list ap_copy2;
    va_copy(ap_copy2, ap);
    int len = redisvFormatCommand(&cmd_buf, format, ap_copy2);
    if (len == -1) {
      printf("Out of memory");
    } else if (len == -2) {
      printf("Invalid format string");
    } else {
      cmd_.assign(cmd_buf, len);
    }
    va_end(ap_copy2);

    free(cmd_buf);

    step_ = UNDEFINED;
  }

  const string& ToString() const { return fmt_cmd_; }

  string key_;
  string cmd_;
  Step step_;
  OnReplyCallback reply_cb_;
  string fmt_cmd_;
};

class RedisReply : public CDBReply {
  public:
  RedisReply(const redisReply* reply = NULL) : redis_reply_(reply) { }
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

class RedisClient : public CDBClient {
  public:
  RedisClient(bool auto_reconnect = true) : CDBClient(auto_reconnect), connected_(false) { }

  bool IsReady() const { return Connected(); }
  bool Connected() const { return connected_; }

  void SetCallbacks(const CDBCallbacksPtr& cdb_cbs) { cdb_cbs_ = cdb_cbs; }

  virtual void* GetCluster() { return cluster_; }
  virtual void SetCluster(void* cluster) { cluster_ = cluster; }

  const char* GetLastError() const { return IsReady() ? m_errstr : ERR_CONNECTION_NOT_READY; }

  void DefaultOnReplyCb(CDBClient* cdbc, const CDBReply* cdb_msg)
  {
      const redisReply* reply = (const redisReply*)cdb_msg->GetReply();
      printf("[RedisClient::DefaultOnReplyCb] received reply: \n"
              "{ type: %d, integer: %lld, len: %ld, str: %s, elements: %lu, element list: %p }\n",
              reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);
  }

  protected:
  bool NeedRetry() { return false; }

  protected:
  bool connected_;
  char m_errstr[256];
  void* cluster_;
};

class RedisAsyncClient : public RedisClient, public IOEvent {
  public:
  RedisAsyncClient(bool auto_reconnect = true) : RedisClient(auto_reconnect), redis_ctx_(NULL) { }
  ~RedisAsyncClient() { Disconnect(); redis_ctx_ = NULL; }
  bool Init(const char* host, uint16_t port, const CDBCallbacksPtr& cdb_cbs = nullptr, bool auto_reconnect = true);

  bool IsReady() const { return redis_ctx_ && redis_ctx_->err == REDIS_OK; }
  void Disconnect();
  void HandleConnect();
  void HandleDisconnect();

  void SetExcepitonReplyCallback(const RedisAskReplyCallback& ask_cb,
          const RedisRedirectReplyCallback& redirect_cb,
          const RedisClusterDownReplyCallback& cluster_down_cb);

  redisAsyncContext* RedisContext() { return redis_ctx_; }

  bool SendCommand(CDBReply* user_reply, const char* format, ...);
  bool SendCommand(CDBReply* user_reply, const char* format, va_list ap);
  bool SendCommand(const OnReplyCallback& reply_cb, const char* format, ...);
  bool SendCommand(const OnReplyCallback& reply_cb, const char* format, va_list ap);
  bool SendCommand(const RedisRequestPtr& request);

  void OnRedisReply(const redisAsyncContext* ctx, redisReply* reply);
  void OnRedisConnect(const redisAsyncContext* ctx, int status);
  void OnRedisDisconnect(const redisAsyncContext* ctx, int status);

  bool HasError() const;

  private:
  bool Connect_(bool reconnect = false);
  bool SetRedisCallbacks();

  void OnEvents(uint32_t events);
  void OnError(CDBClient* cdbclient, int errcode, const char* errstr);
  void DefaultOnReplyCb(CDBClient* cdbc, const CDBReply* cdb_msg);
  RedisRequest::Step HandleReply(const RedisRequestPtr& request, const RedisReply& reply);

  private:
  redisAsyncContext*    redis_ctx_;
  RedisRequestQueue     request_queue_;

  RedisAskReplyCallback         ask_cb_;
  RedisRedirectReplyCallback    redirect_cb_;
  RedisClusterDownReplyCallback cluster_down_cb_;
};

class RedisSyncClient : public RedisClient {
  public:
  RedisSyncClient(bool auto_reconnect = true) : RedisClient(auto_reconnect), redis_ctx_(NULL) { }
  ~RedisSyncClient() { Disconnect(); redis_ctx_ = NULL; }

  bool IsReady() const { return redis_ctx_ && redis_ctx_->err == REDIS_OK; }
  void Disconnect();

  bool SendCommand(CDBReply* user_reply, const char* format, ...);
  bool SendCommand(CDBReply* user_reply, const char* format, va_list ap);
  bool SendCommand(CDBReply* user_reply, const RedisRequestPtr& request);
  bool SendCommand(const OnReplyCallback& reply_cb, const char* format, ...);

  bool HasError() const;

  private:
  bool Connect_(bool reconnect = false);
  RedisRequest::Step HandleReply(CDBReply* user_reply, const RedisRequestPtr& request, const RedisReply& reply);
  void SendAskRequest(CDBReply* user_reply, const string& ip, uint16_t port);
  void SendRedirectRequest(CDBReply* user_reply, const RedisRequestPtr& request, const string& ip, uint16_t port);

  private:
  redisContext*   redis_ctx_;
};

}  // namespace cdb_api

#endif  // _CDB_REDIS_H

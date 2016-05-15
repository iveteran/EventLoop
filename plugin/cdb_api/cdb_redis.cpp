#include "cdb_redis.h"

namespace cdb_api {

/*
 * Implements of RedisAsyncClient
 */
static void __RedisEventloopAddReadEvent(void * adapter)
{
  RedisAsyncClient* rac = static_cast<RedisAsyncClient *>(adapter);
  rac->AddReadEvent();
}
static void __RedisEventloopDelReadEvent(void * adapter)
{
  RedisAsyncClient* rac = static_cast<RedisAsyncClient *>(adapter);
  rac->DeleteReadEvent();
}
static void __RedisEventloopAddWriteEvent(void * adapter)
{
  RedisAsyncClient* rac = static_cast<RedisAsyncClient *>(adapter);
  rac->AddWriteEvent();
}
static void __RedisEventloopDelWriteEvent(void * adapter)
{
  RedisAsyncClient* rac = static_cast<RedisAsyncClient *>(adapter);
  rac->DeleteWriteEvent();
}
static void __RedisEventloopCleanupEvent(void * adapter)
{
  RedisAsyncClient* rac = static_cast<RedisAsyncClient *>(adapter);
  rac->ClearAllEvents();
}

static void __GetReplyCallback(redisAsyncContext *ctx, void * r, void * privdata)
{
  redisReply* reply = static_cast<redisReply *>(r);
  RedisAsyncClient* rac = static_cast<RedisAsyncClient *>(privdata);
  if (reply == nullptr || rac == nullptr) return;

  rac->OnRedisReply(ctx, reply);
}
static void __RedisConnectCallback(const redisAsyncContext *ctx, int status)
{
  RedisAsyncClient* rac = static_cast<RedisAsyncClient *>(ctx->ev.data);
  rac->OnRedisConnect(ctx, status);
}
static void __RedisDisconnectCallback(const redisAsyncContext *ctx, int status)
{
  RedisAsyncClient* rac = static_cast<RedisAsyncClient *>(ctx->ev.data);
  rac->OnRedisDisconnect(ctx, status);
}


bool RedisAsyncClient::Init(const char* host, uint16_t port, const CDBCallbacksPtr& cdb_cbs, bool auto_reconnect)
{
  CDBClient::Init(host, port, auto_reconnect);
  cdb_cbs_ = cdb_cbs;

  return SendCommand(NULL, "ping");  // trigger callback OnRedisConnect
}

bool RedisAsyncClient::IsReady()
{
  return redis_ctx_ && redis_ctx_->err == REDIS_OK;
}

void RedisAsyncClient::Disconnect()
{
  if (redis_ctx_) {
    //redisAsyncDisconnect(redis_ctx_);
    redis_ctx_->ev.data = NULL;
    redis_ctx_ = NULL;
  }
}

bool RedisAsyncClient::SendCommand(CDBMessage* msg, const char* format, ...)
{
  UNUSED(msg);
  OnReplyCallback default_on_reply_cb = std::bind(&RedisAsyncClient::DefaultOnReplyCb, this, std::placeholders::_1, std::placeholders::_2);
  reply_cb_queue_.push(default_on_reply_cb);
  va_list ap;
  va_start(ap, format);
  int status = redisvAsyncCommand(redis_ctx_, __GetReplyCallback, this, format, ap);
  va_end(ap);
  return status == REDIS_OK;
}

bool RedisAsyncClient::SendCommand(const OnReplyCallback& reply_cb, const char* format, ...)
{
  reply_cb_queue_.push(reply_cb);
  va_list ap;
  va_start(ap, format);
  int status = redisvAsyncCommand(redis_ctx_, __GetReplyCallback, this, format, ap);
  va_end(ap);
  return status == REDIS_OK;
}

bool RedisAsyncClient::Connect_(bool reconnect)
{
  UNUSED(reconnect);
  redisAsyncContext* ctx = redisAsyncConnect(server_addr_.ip_.c_str(), server_addr_.port_);
  if (ctx && ctx->err) {
    printf("[RedisAsyncClient::Connect_] Error: %s\n", ctx->errstr);
    redisAsyncFree(ctx);
    return false;
  }
  return SetContext(ctx) == REDIS_OK;
}

int RedisAsyncClient::SetContext(redisAsyncContext * ctx)
{
  if (ctx->ev.data != NULL) {
    return REDIS_ERR;
  }

  redis_ctx_ = ctx;
  redis_ctx_->ev.data = this;
  redis_ctx_->ev.addRead = __RedisEventloopAddReadEvent;
  redis_ctx_->ev.delRead = __RedisEventloopDelReadEvent;
  redis_ctx_->ev.addWrite = __RedisEventloopAddWriteEvent;
  redis_ctx_->ev.delWrite = __RedisEventloopDelWriteEvent;
  redis_ctx_->ev.cleanup = __RedisEventloopCleanupEvent;

  redisAsyncSetConnectCallback(redis_ctx_, __RedisConnectCallback);
  redisAsyncSetDisconnectCallback(redis_ctx_, __RedisDisconnectCallback);

  SetFD(redis_ctx_->c.fd);

  return REDIS_OK;
}

void RedisAsyncClient::OnEvents(uint32_t events)
{
  //printf("[RedisAsyncClient::OnEvents] events: %d\n", events);
  if (events & IOEvent::WRITE) {
    redisAsyncHandleWrite(redis_ctx_); 
  }
  if (events & IOEvent::READ) {
    redisAsyncHandleRead(redis_ctx_);
  }
  if (events & IOEvent::ERROR) {
    OnError(errno, strerror(errno));
  }
}

void RedisAsyncClient::OnRedisReply(const redisAsyncContext* ctx, redisReply* reply)
{
  /*
  printf("[RedisAsyncClient::OnRedisReply] received reply, fd: %d\n"
      " reply: { type: %d, integer: %d, len: %d, str: %s, elements: %d, element list: %x }\n",
      ctx->c.fd, reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);
  */
  RedisMessage rmsg(reply);
  auto& reply_cb = reply_cb_queue_.front();
  reply_cb(this, &rmsg);
  reply_cb_queue_.pop();
}
void RedisAsyncClient::DefaultOnReplyCb(CDBClient* cdbc, const CDBMessage* cdb_msg)
{
    const redisReply* reply = (const redisReply*)cdb_msg->GetReply();
    printf("[DASMsgHandler::DefaultOnReplyCb] received reply: \n"
            "{ type: %d, integer: %lld, len: %d, str: %s, elements: %lu, element list: %p }\n",
            reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);
}

void RedisAsyncClient::SetCallbacks(const CDBCallbacksPtr& cdb_cbs)
{
  cdb_cbs_ = cdb_cbs;
}

void RedisAsyncClient::OnRedisConnect(const redisAsyncContext* ctx, int status)
{
  printf("[RedisAsyncClient::OnRedisConnect] connection fd: %d, status: %d\n", ctx->c.fd, status);
  if (status == 0) {
    if (cdb_cbs_) cdb_cbs_->on_connected_cb(this);
  } else {
    Reconnect();
  }
}
void RedisAsyncClient::OnRedisDisconnect(const redisAsyncContext* ctx, int status)
{
  printf("[RedisAsyncClient::OnRedisDisconnect] connection lost, fd: %d, status: %d\n", ctx->c.fd, status);
  if (cdb_cbs_) cdb_cbs_->on_closed_cb(this);
  if (auto_reconnect_) {
    Reconnect();
  }
}
void RedisAsyncClient::OnError(int errcode, const char* errstr)
{
  printf("[RedisAsyncClient::OnError] error code: %d, error string: %s\n", errcode, errstr);
  if (cdb_cbs_) cdb_cbs_->on_error_cb(errcode, errstr);
}

/*
 * Implements of RedisClient
 */
bool RedisClient::IsReady()
{
  return redis_ctx_ && redis_ctx_->err == REDIS_OK;
}

void RedisClient::Disconnect()
{
  if (redis_ctx_) {
    redisFree(redis_ctx_);
    redis_ctx_ = NULL;
  }
}

bool RedisClient::SendCommand(CDBMessage* msg, const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  void* reply = redisvCommand(redis_ctx_, format, ap);
  va_end(ap);
  if (msg && reply) msg->SetReply(reply);
  return msg != NULL;
}

bool RedisClient::SendCommand(const OnReplyCallback& reply_cb, const char* format, ...)
{
  return false;
}

bool RedisClient::Connect_(bool reconnect)
{
  if (reconnect && redis_ctx_) {
    int status = redisReconnect(redis_ctx_);
    if (status != REDIS_OK) {
      printf("[RedisClient::Connect_] Reconnect failed: %s\n", redis_ctx_->errstr);
      return false;
    }
  } else {
    redis_ctx_ = redisConnect(server_addr_.ip_.c_str(), server_addr_.port_);
    if (redis_ctx_ && redis_ctx_->err) {
      printf("[RedisClient::Connect_] Connect failed: %s\n", redis_ctx_->errstr);
      return false;
    }
  }
  return true;
}


}  // namespace db_api

#include "hiredis_adapter.h"

namespace evt_loop {

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

RedisAsyncClient::RedisAsyncClient(const char* host, uint16_t port, bool auto_reconnect, RedisCallbacksPtr redis_cbs) :
  redis_ctx_(NULL), auto_reconnect_(auto_reconnect), reconnect_timer_(this), redis_cbs_(redis_cbs)
{
  server_addr_.port_ = port;
  if (host[0] == '\0' || strcmp(host, "localhost") == 0) {
    server_addr_.ip_ = "127.0.0.1";
  } else if (!strcmp(host, "any")) {
    server_addr_.ip_ = "0.0.0.0";
  } else {
    server_addr_.ip_ = host;
  }
  if (auto_reconnect_) {
    timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    reconnect_timer_.SetInterval(tv);
  }

  Connect();
  SendCommand("ping");  // trigger callback OnRedisConnect
}

RedisAsyncClient::~RedisAsyncClient()
{
  if (redis_ctx_ != 0) {
    redis_ctx_->ev.data = NULL;
  }
  Disconnect();
}

bool RedisAsyncClient::Connect()
{
  bool success = Connect_();
  if (!success && auto_reconnect_)
    Reconnect();
  return success;
}

void RedisAsyncClient::Disconnect()
{
  if (redis_ctx_) {
    //redisAsyncDisconnect(redis_ctx_);
    redis_ctx_ = NULL;
  }
}

void RedisAsyncClient::Reconnect()
{
  //Disconnect();
  if (!reconnect_timer_.IsRunning())
    reconnect_timer_.Start(EV_Singleton);
}

/*
bool RedisAsyncClient::SendCommand(const string& msg)
{
  bool success = true;
  if (redis_ctx_) {
    redisAsyncCommand(redis_ctx_, __GetReplyCallback, this, msg.data());
  } else {
    tmp_sendbuf_list_.push_back(msg);
  }
  return success;
}
*/

bool RedisAsyncClient::SendCommand(const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  int status = redisvAsyncCommand(redis_ctx_, __GetReplyCallback, this, format, ap);
  va_end(ap);
  return status == REDIS_OK;
}

void RedisAsyncClient::SetRedisCallbacks(const RedisCallbacksPtr& redis_cbs)
{
  redis_cbs_ = redis_cbs;
}

bool RedisAsyncClient::Connect_()
{
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
  EV_Singleton->AddEvent(this);

  return REDIS_OK;
}

/*
void RedisAsyncClient::SendTempBuffer()
{
  if (redis_ctx_ == NULL) return;

  while (!tmp_sendbuf_list_.empty()) {
    const string& sendbuf = tmp_sendbuf_list_.front();
    SendCommand(sendbuf);
    tmp_sendbuf_list_.pop_front();
  }
}
*/

void RedisAsyncClient::OnEvents(uint32_t events)
{
  printf("[RedisAsyncClient::OnEvents] events: %d\n", events);
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
  if (redis_cbs_) redis_cbs_->on_msg_recvd_cb(this, &rmsg);
}
void RedisAsyncClient::OnRedisConnect(const redisAsyncContext* ctx, int status)
{
  printf("[RedisAsyncClient::OnRedisConnect] connection fd: %d, status: %d\n", ctx->c.fd, status);
  if (status == 0) {
    //SendTempBuffer();
    if (redis_cbs_) redis_cbs_->on_new_client_cb(this);
  } else {
    Reconnect();
  }
}
void RedisAsyncClient::OnRedisDisconnect(const redisAsyncContext* ctx, int status)
{
  printf("[RedisAsyncClient::OnRedisDisconnect] connection lost, fd: %d, status: %d\n", ctx->c.fd, status);
  if (redis_cbs_) redis_cbs_->on_closed_cb(this);
  if (auto_reconnect_) {
    Reconnect();
  }
}
void RedisAsyncClient::OnError(int errcode, const char* errstr)
{
  printf("[RedisAsyncClient::OnError] error code: %d, error string: %s\n", errcode, errstr);
  if (redis_cbs_) redis_cbs_->on_error_cb(errcode, errstr);
}

void RedisAsyncClient::ReconnectTimer::OnTimer()
{
  if (creator_->redis_ctx_ == NULL || creator_->redis_ctx_->err != REDIS_OK) {  // if the connection is not created, then reconnect
    bool success = creator_->Connect_();
    if (success)
      creator_->reconnect_timer_.Stop();
    else
      printf("[RedisAsyncClient::ReconnectTimer::OnTimer] Reconnect failed, retry %u seconds later...\n", GetInterval().Seconds());
  } else {
    creator_->reconnect_timer_.Stop();
  }
}

}  // namespace evt_loop

#include "cdb_redis.h"

namespace cdb_api {

const char* ERR_CONNECTION_NOT_READY = "RedisClient: client is not connected";

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

RedisAsyncClient::~RedisAsyncClient()
{
  Disconnect();
  redis_ctx_ = NULL;
}

bool RedisAsyncClient::Init(const char* host, uint16_t port, const CDBCallbacksPtr& cdb_cbs, bool auto_reconnect)
{
  redis_ctx_ = redisAsyncContextCreate_ext();
  SetRedisCallbacks();

  return CDBClient::Init(host, port, cdb_cbs, auto_reconnect);
}

bool RedisAsyncClient::IsReady() const
{
  return Connected();
}

bool RedisAsyncClient::Connected() const
{
  return connected_;
}

void RedisAsyncClient::HandleConnect()
{
  printf("[RedisAsyncClient::HandleConnect] Connect to redis server successful, fd: %d\n", redis_ctx_->c.fd);
  SetFD(redis_ctx_->c.fd);
  connected_ = true;
  SendCommand(NULL, "PING");  // just to check the connection whether available
  if (cdb_cbs_) cdb_cbs_->on_connected_cb(this);
}
void RedisAsyncClient::HandleDisconnect()
{
  SetFD(-1);
  connected_ = false;
}

void RedisAsyncClient::Disconnect()
{
  if (redis_ctx_) {
    redisAsyncDisconnect(redis_ctx_);
  }
}

bool RedisAsyncClient::SendCommand(CDBReply* msg, const char* format, ...)
{
  UNUSED(msg);
  if (!IsReady()) { return false; }

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
  if (!IsReady()) { return false; }

  reply_cb_queue_.push(reply_cb);
  va_list ap;
  va_start(ap, format);
  int status = redisvAsyncCommand(redis_ctx_, __GetReplyCallback, this, format, ap);
  va_end(ap);
  return status == REDIS_OK;
}

bool RedisAsyncClient::Connect_(bool reconnect)
{
  int status = REDIS_ERR;
  if (!reconnect)
    status = redisAsyncConnectBlock_ext(redis_ctx_, server_addr_.ip_.c_str(), server_addr_.port_);
  else
    status = redisAsyncReconnectBlock_ext(redis_ctx_);

  if (status == REDIS_OK) {
    HandleConnect();
  } else {
    printf("[RedisAsyncClient::Connect_] redis.err: %d, redis.errstr: %s, errno: %d, errstr: %s\n",
            redis_ctx_->err, redis_ctx_->errstr, errno, strerror(errno));
    connected_ = false;
    OnError(this, redis_ctx_->err, redis_ctx_->errstr);
  }

  return connected_;
}

bool RedisAsyncClient::SetRedisCallbacks()
{
  if (redis_ctx_->ev.data != NULL)
    return false;

  redis_ctx_->ev.data = this;
  redis_ctx_->ev.addRead = __RedisEventloopAddReadEvent;
  redis_ctx_->ev.delRead = __RedisEventloopDelReadEvent;
  redis_ctx_->ev.addWrite = __RedisEventloopAddWriteEvent;
  redis_ctx_->ev.delWrite = __RedisEventloopDelWriteEvent;
  redis_ctx_->ev.cleanup = __RedisEventloopCleanupEvent;

  redisAsyncSetConnectCallback(redis_ctx_, __RedisConnectCallback);
  redisAsyncSetDisconnectCallback(redis_ctx_, __RedisDisconnectCallback);

  return true;
}

void RedisAsyncClient::OnEvents(uint32_t events)
{
  //printf("[RedisAsyncClient::OnEvents] events: %d, redis.errcode: %d\n", events, redis_ctx_->err);
  if (events & IOEvent::WRITE) {
    redisAsyncHandleWrite(redis_ctx_); 
  }
  if (events & IOEvent::READ) {
    redisAsyncHandleRead(redis_ctx_);
  }
  if (HasError()) {
    if (redis_ctx_->err == REDIS_ERR_EOF) {
      HandleDisconnect();
      OnError(this, redis_ctx_->err, redis_ctx_->errstr);
    } else if (redis_ctx_->err == REDIS_ERR_IO) {
      OnError(this, errno, strerror(errno));
    }
  }
  if (events & IOEvent::ERROR) {
    OnError(this, errno, strerror(errno));
  }
}

void RedisAsyncClient::OnRedisReply(const redisAsyncContext* ctx, redisReply* reply)
{
  /*
  printf("[RedisAsyncClient::OnRedisReply] received reply, fd: %d\n"
      " reply: { type: %d, integer: %lld, len: %ld, str: %s, elements: %lu, element list: %p }\n",
      ctx->c.fd, reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);
  */
  RedisReply rmsg(reply);
  auto& reply_cb = reply_cb_queue_.front();
  reply_cb(this, &rmsg);
  reply_cb_queue_.pop();
}
void RedisAsyncClient::DefaultOnReplyCb(CDBClient* cdbc, const CDBReply* cdb_msg)
{
    const redisReply* reply = (const redisReply*)cdb_msg->GetReply();
    printf("[DASMsgHandler::DefaultOnReplyCb] received reply: \n"
            "{ type: %d, integer: %lld, len: %ld, str: %s, elements: %lu, element list: %p }\n",
            reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);
}

void RedisAsyncClient::SetCallbacks(const CDBCallbacksPtr& cdb_cbs)
{
  cdb_cbs_ = cdb_cbs;
}

void RedisAsyncClient::OnRedisConnect(const redisAsyncContext* ctx, int status)
{
  if (status == REDIS_OK) {
    HandleConnect();
  } else {
    printf("[RedisAsyncClient::OnRedisConnect] fd: %d, status: %d, redis.errcode: %d, redis.errstr: %s\n",
            ctx->c.fd, status, ctx->err, ctx->errstr);
    connected_ = false;
    OnError(this, ctx->err, ctx->errstr);
    Reconnect();
  }
}
void RedisAsyncClient::OnRedisDisconnect(const redisAsyncContext* ctx, int status)
{
  printf("[RedisAsyncClient::OnRedisDisconnect] connection lost, fd: %d, status: %d\n", ctx->c.fd, status);
  HandleDisconnect();
  if (cdb_cbs_) cdb_cbs_->on_closed_cb(this);
  if (auto_reconnect_) {
    Reconnect();
  }
}
void RedisAsyncClient::OnError(CDBClient* cdbclient, int errcode, const char* errstr)
{
  printf("[RedisAsyncClient::OnError] error code: %d, error string: %s\n", errcode, errstr);
  snprintf(m_errstr, sizeof(m_errstr), "RedisAsyncClient(%d): %s", errcode, errstr);
  if (cdb_cbs_) cdb_cbs_->on_error_cb(cdbclient, errcode, errstr);
}
bool RedisAsyncClient::HasError() const
{
  return redis_ctx_ && redis_ctx_->err != REDIS_OK;
}
const char* RedisAsyncClient::GetLastError() const
{
  return IsReady() ? m_errstr : ERR_CONNECTION_NOT_READY;
}


/*
 * Implements of RedisClient
 */
bool RedisClient::IsReady() const
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

bool RedisClient::SendCommand(CDBReply* msg, const char* format, ...)
{
  if (!IsReady()) { return false; }

  va_list ap;
  va_start(ap, format);
  void* reply = redisvCommand(redis_ctx_, format, ap);
  va_end(ap);
  if (msg && reply) msg->SetReply(reply);
  return msg != NULL;
}

bool RedisClient::SendCommand(const OnReplyCallback& reply_cb, const char* format, ...)
{
  snprintf(m_errstr, sizeof(m_errstr), "RedisClient: api not implemented");
  return false;
}

bool RedisClient::Connect_(bool reconnect)
{
  if (reconnect && redis_ctx_) {
    int status = redisReconnect(redis_ctx_);
    if (status != REDIS_OK) {
      printf("[RedisClient::Connect_] Reconnect failed: %s\n", redis_ctx_->errstr);
      snprintf(m_errstr, sizeof(m_errstr), "RedisClient(%d): %s", redis_ctx_->err, redis_ctx_->errstr);
      return false;
    }
  } else {
    redis_ctx_ = redisConnect(server_addr_.ip_.c_str(), server_addr_.port_);
    if (redis_ctx_ && redis_ctx_->err) {
      printf("[RedisClient::Connect_] Connect failed: %s\n", redis_ctx_->errstr);
      snprintf(m_errstr, sizeof(m_errstr), "RedisClient(%d): %s", redis_ctx_->err, redis_ctx_->errstr);
      return false;
    }
  }
  if (cdb_cbs_) cdb_cbs_->on_connected_cb(this);
  return true;
}

bool RedisClient::HasError() const
{
  return redis_ctx_ && redis_ctx_->err != REDIS_OK;
}
const char* RedisClient::GetLastError() const
{
  return IsReady() ? m_errstr : ERR_CONNECTION_NOT_READY;
}

}  // namespace db_api

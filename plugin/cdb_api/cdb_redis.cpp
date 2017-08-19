#include "cdb_redis.h"
#include "cdb_redis_cluster.h"

namespace cdb_api {

string GetRedisKeyFromCommand(char* cmd_str)
{
  char* token = NULL;
  const char* sep = " ";
  token = strtok(cmd_str, sep);
  if (token)
      token = strtok(NULL, sep);
  return token ? string(token) : string();
}
string GetRedisKeyFromCommand(const char* format, va_list ap, string* fmt_cmd)
{
  char cmd_str[1024] = {0};
  vsnprintf(cmd_str, sizeof(cmd_str), format, ap);
  if (fmt_cmd) {
    *fmt_cmd = cmd_str;
  }
  return GetRedisKeyFromCommand(cmd_str);
}

static ReplyState ProcessResult(int result_code, const char* result_desc, int result_desc_len)
{
  ReplyState state = ReplyState::NORMAL;
  if (result_code == REDIS_REPLY_ERROR ) {
    if (strstr(result_desc, "ASK") != NULL) {
      state = ReplyState::ASK;
    } else if (strstr(result_desc, "MOVED") != NULL) {
      state = ReplyState::MOVED;
    } else if (strstr(result_desc, "CLUSTERDOWN") != NULL) {
      state = ReplyState::CLUSTERDOWN;
    }
  }
  return state;
}
static bool ParseNodeAddress(string desc, string& host, uint16_t& port)
{
  size_t hostPotision = desc.find( " ", desc.find( " ") + 1) + 1;
  size_t portPosition = desc.find( ":", hostPotision) + 1;

  if( hostPotision != string::npos && portPosition != string::npos) {
    host = desc.substr(hostPotision, portPosition - hostPotision - 1);
    port = atol(desc.substr(portPosition).c_str());
    return true;
  }
  return false;
}

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
  redis_ctx_ = redisAsyncContextCreate_ext();
  SetRedisCallbacks();

  return CDBClient::Init(host, port, cdb_cbs, auto_reconnect);
}
void RedisAsyncClient::HandleConnect()
{
  printf("[RedisAsyncClient::HandleConnect] Connect to redis server(%s) successful, fd: %d\n", server_addr_.ToString().c_str(), redis_ctx_->c.fd);
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
void RedisAsyncClient::SetExcepitonReplyCallback(const RedisAskReplyCallback& ask_cb,
        const RedisRedirectReplyCallback& redirect_cb,
        const RedisClusterDownReplyCallback& cluster_down_cb)
{
  ask_cb_ = ask_cb;
  redirect_cb_ = redirect_cb;
  cluster_down_cb_ = cluster_down_cb;
}
RedisRequest::Step RedisAsyncClient::HandleReply(const RedisRequestPtr& request, const RedisReply& reply)
{
  RedisRequest::Step step = RedisRequest::Step::FAILED;
  const redisReply* r_reply = (const redisReply*)reply.GetReply();

  ReplyState reply_state = ProcessResult(r_reply->type, r_reply->str, r_reply->len);
  switch (reply_state) {
    case ReplyState::ASK:
    case ReplyState::MOVED:
      {
        string host;
        uint16_t port;
        bool success = ParseNodeAddress(r_reply->str, host, port);
        if (success) {
          if (reply_state == ReplyState::ASK) {
            step = RedisRequest::Step::ASK;
            if (ask_cb_) ask_cb_(this, request, host, port);
          } else {
            step = RedisRequest::Step::REDIRECT;
            if (redirect_cb_) redirect_cb_(this, request, host, port);
          }
        } else {
          step = RedisRequest::Step::FAILED;
        }
      }
      break;
    case ReplyState::CLUSTERDOWN:
      step = RedisRequest::Step::FAILED;
      if (cluster_down_cb_) cluster_down_cb_(this, request);
      break;
    case ReplyState::NORMAL:
      {
        step = RedisRequest::Step::FINISH;
        if (request->reply_cb_) request->reply_cb_(this, &reply);
      }
      break;
    default:
      step = RedisRequest::Step::FAILED;
      break;
  }
  if (step == RedisRequest::Step::FAILED && NeedRetry()) {
    //retry(con, r, data );
  }
  return step;
}
bool RedisAsyncClient::SendCommand(CDBReply* user_reply, const char* format, ...)
{
  UNUSED(user_reply);

  va_list ap;
  va_start(ap, format);
  bool success = SendCommand(user_reply, format, ap);
  va_end(ap);
  return success;
}
bool RedisAsyncClient::SendCommand(CDBReply* user_reply, const char* format, va_list ap)
{
  UNUSED(user_reply);
  if (!IsReady()) { return false; }

  OnReplyCallback reply_cb = std::bind(&RedisClient::DefaultOnReplyCb, this, std::placeholders::_1, std::placeholders::_2);
  va_list ap_copy;
  va_copy(ap_copy, ap);
  auto request = std::make_shared<RedisRequest>(reply_cb, format, ap_copy);
  va_end(ap_copy);
  printf("[RedisAsyncClient::SendCommand] cmd: %s\n", request->ToString().c_str());
  request_queue_.push(request);
  int status = redisAsyncFormattedCommand(redis_ctx_, __GetReplyCallback, this, request->cmd_.data(), request->cmd_.size());
  return status == REDIS_OK;
}
bool RedisAsyncClient::SendCommand(const OnReplyCallback& reply_cb, const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  bool success = SendCommand(reply_cb, format, ap);
  va_end(ap);
  return success;
}
bool RedisAsyncClient::SendCommand(const OnReplyCallback& reply_cb, const char* format, va_list ap)
{
  if (!IsReady()) { return false; }

  va_list ap_copy;
  va_copy(ap_copy, ap);
  auto request = std::make_shared<RedisRequest>(reply_cb, format, ap_copy);
  va_end(ap_copy);
  printf("[RedisAsyncClient::SendCommand] cmd: %s\n", request->ToString().c_str());
  request_queue_.push(request);
  int status = redisAsyncFormattedCommand(redis_ctx_, __GetReplyCallback, this, request->cmd_.data(), request->cmd_.size());
  return status == REDIS_OK;
}
bool RedisAsyncClient::SendCommand(const RedisRequestPtr& request)
{
  printf("[RedisAsyncClient::SendCommand] cmd: %s\n", request->ToString().c_str());
  request_queue_.push(request);
  int status = redisAsyncFormattedCommand(redis_ctx_, __GetReplyCallback, this, request->cmd_.data(), request->cmd_.size());
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
  if (events & FileEvent::WRITE) {
    redisAsyncHandleWrite(redis_ctx_); 
  }
  if (events & FileEvent::READ) {
    redisAsyncHandleRead(redis_ctx_);
  }
  if (events & FileEvent::CLOSED) {
    HandleDisconnect();
  }
  if (HasError()) {
    if (redis_ctx_->err == REDIS_ERR_EOF) {
      HandleDisconnect();
    } else if (redis_ctx_->err == REDIS_ERR_IO) {
      OnError(this, errno, strerror(errno));
    }
  }
  if (events & FileEvent::ERROR) {
    OnError(this, errno, strerror(errno));
  }
}

void RedisAsyncClient::OnRedisReply(const redisAsyncContext* ctx, redisReply* reply)
{
  printf("[RedisAsyncClient::OnRedisReply] received reply, fd: %d\n"
      " reply: { type: %d, integer: %lld, len: %ld, str: %s, elements: %lu, element list: %p }\n",
      ctx->c.fd, reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);

  if (!request_queue_.empty()) {
    auto& request = request_queue_.front();
    RedisReply redis_reply(reply);
    HandleReply(request, redis_reply);
    request_queue_.pop();
  }
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

void RedisSyncClient::Disconnect()
{
  if (redis_ctx_) {
    redisFree(redis_ctx_);
    redis_ctx_ = NULL;
  }
}

bool RedisSyncClient::SendCommand(CDBReply* user_reply, const char* format, ...)
{
  if (!IsReady()) { return false; }

  va_list ap;
  va_start(ap, format);
  bool success = SendCommand(user_reply, format, ap);
  va_end(ap);
  return success;
}
bool RedisSyncClient::SendCommand(CDBReply* user_reply, const char* format, va_list ap)
{
  if (!IsReady()) { return false; }

  auto request = std::make_shared<RedisRequest>(nullptr, format, ap);
  return SendCommand(user_reply, request);
}
bool RedisSyncClient::SendCommand(CDBReply* user_reply, const RedisRequestPtr& request)
{
  printf("[RedisSyncClient::SendCommand] cmd: %s, len: %ld\n", request->ToString().c_str(), request->cmd_.size());
  if (!IsReady()) { return false; }

  bool success = false;
  int status = redisAppendFormattedCommand(redis_ctx_, request->cmd_.data(), request->cmd_.size());
  if (status == REDIS_OK) {
    void* r_reply = NULL;
    status = redisGetReply(redis_ctx_, &r_reply);
    if (status == REDIS_OK && r_reply != NULL) {
      RedisReply redis_reply((redisReply*)r_reply);
      HandleReply(user_reply, request, redis_reply);
      success = true;
    }
  }
  return success;
}
bool RedisSyncClient::SendCommand(const OnReplyCallback& reply_cb, const char* format, ...)
{
  snprintf(m_errstr, sizeof(m_errstr), "[RedisSyncClient::SendCommand]: api not implemented");
  return false;
}

bool RedisSyncClient::Connect_(bool reconnect)
{
  if (reconnect && redis_ctx_) {
    int status = redisReconnect(redis_ctx_);
    if (status != REDIS_OK) {
      printf("[RedisSyncClient::Connect_] Reconnect failed: %s\n", redis_ctx_->errstr);
      snprintf(m_errstr, sizeof(m_errstr), "RedisSyncClient(%d): %s", redis_ctx_->err, redis_ctx_->errstr);
      return false;
    }
  } else {
    redis_ctx_ = redisConnect(server_addr_.ip_.c_str(), server_addr_.port_);
    if (redis_ctx_ && redis_ctx_->err) {
      printf("[RedisSyncClient::Connect_] Connect failed: %s\n", redis_ctx_->errstr);
      snprintf(m_errstr, sizeof(m_errstr), "RedisSyncClient(%d): %s", redis_ctx_->err, redis_ctx_->errstr);
      return false;
    }
  }
  if (cdb_cbs_) cdb_cbs_->on_connected_cb(this);
  return true;
}

bool RedisSyncClient::HasError() const
{
  return redis_ctx_ && redis_ctx_->err != REDIS_OK;
}

RedisRequest::Step RedisSyncClient::HandleReply(CDBReply* user_reply, const RedisRequestPtr& request, const RedisReply& reply)
{
  RedisRequest::Step step = RedisRequest::Step::FAILED;
  const redisReply* r_reply = (const redisReply*)reply.GetReply();

  ReplyState reply_state = ProcessResult(r_reply->type, r_reply->str, r_reply->len);
  switch (reply_state) {
    case ReplyState::ASK:
    case ReplyState::MOVED:
      {
        string host;
        uint16_t port;
        bool success = ParseNodeAddress(r_reply->str, host, port);
        if (success) {
          if (reply_state == ReplyState::ASK) {
            step = RedisRequest::Step::ASK;
            SendAskRequest(user_reply, host, port);
          } else {
            step = RedisRequest::Step::REDIRECT;
            SendRedirectRequest(user_reply, request, host, port);
          }
        } else {
          step = RedisRequest::Step::FAILED;
          user_reply->SetReply(reply.GetReply());
        }
      }
      break;
    case ReplyState::CLUSTERDOWN:
      step = RedisRequest::Step::FAILED;
    case ReplyState::NORMAL:
      step = RedisRequest::Step::FINISH;
    default:
      step = RedisRequest::Step::FAILED;
      user_reply->SetReply(reply.GetReply());
      break;
  }
  if (step == RedisRequest::Step::FAILED && NeedRetry()) {
    //retry(con, r, data );
  }
  return step;
}
void RedisSyncClient::SendAskRequest(CDBReply* user_reply, const string& ip, uint16_t port)
{
  printf("[RedisSyncClient::SendAskRequest] Asking to node: %s:%d\n", ip.c_str(), port);
  RedisClusterSync* cluster = (RedisClusterSync*)GetCluster();
  if (cluster) {
    auto target_node = cluster->GetNode(ip, port);
    if (!target_node) {
      target_node = cluster->AddNode(ip, port, cluster->IsAutoReconnect());
      //SetNodeExceptionReplyCallback(target_node.get());
    }
    target_node->SendCommand(user_reply, "ASKING");
  }
}
void RedisSyncClient::SendRedirectRequest(CDBReply* user_reply, const RedisRequestPtr& request, const string& ip, uint16_t port)
{
  printf("[RedisSyncClient::SendRedirectRequest] Redirect to node: %s:%d\n", ip.c_str(), port);
  RedisClusterSync* cluster = (RedisClusterSync*)GetCluster();
  if (cluster) {
    auto target_node = cluster->GetNode(ip, port);
    if (!target_node) {
      target_node = cluster->AddNode(ip, port, cluster->IsAutoReconnect());
      //SetNodeExceptionReplyCallback(target_node.get());
    }
    target_node->SendCommand(user_reply, request);
  }
}

}  // namespace db_api

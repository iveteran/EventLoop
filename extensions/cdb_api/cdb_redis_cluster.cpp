#include "cdb_redis_cluster.h"
#include "eventloop/logger.h"

namespace cdb_api {

void RedisClusterClient::OnNodeConnected(CDBClient* node)
{
  if (++connected_nodes_ == 1) {
    auto reply_cb = std::bind(&RedisClusterClient::HandleClusterSlotsReply, this, std::placeholders::_1, std::placeholders::_2);
    node->SendCommand(reply_cb, "CLUSTER SLOTS");
  }

  if (user_cdb_cbs_ && user_cdb_cbs_->on_connected_cb)
    user_cdb_cbs_->on_connected_cb(node);
}
void RedisClusterClient::OnNodeDisconnected(CDBClient* node)
{
  --connected_nodes_;
  if (user_cdb_cbs_ && user_cdb_cbs_->on_closed_cb)
    user_cdb_cbs_->on_closed_cb(node);
}
void RedisClusterClient::OnNodeErrorOccurred(CDBClient* node, int errcode, const char* errstr)
{
  if (user_cdb_cbs_ && user_cdb_cbs_->on_error_cb)
    user_cdb_cbs_->on_error_cb(node, errcode, errstr);
}
void RedisClusterClient::SetClusterNodeCallbacks()
{
  CDBCallbacksPtr cluster_cdb_cbs = std::make_shared<CDBCallbacks>();
  cluster_cdb_cbs->on_connected_cb = std::bind(&RedisClusterClient::OnNodeConnected, this, std::placeholders::_1);
  cluster_cdb_cbs->on_closed_cb = std::bind(&RedisClusterClient::OnNodeDisconnected, this, std::placeholders::_1);
  cluster_cdb_cbs->on_error_cb = std::bind(&RedisClusterClient::OnNodeErrorOccurred, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
  SetConnectionEventsCallback(cluster_cdb_cbs);
}


bool RedisClusterAsyncClient::Init(const IPAddressList& addr_list, const CDBCallbacksPtr& cdb_cbs, bool auto_reconnect)
{
  user_cdb_cbs_ = cdb_cbs;
  cluster_.SetAutoReconnect(auto_reconnect);
  SetClusterNodeCallbacks();

  for (const auto& addr : addr_list) {
    auto node = cluster_.AddNode(addr.ip_, addr.port_, cluster_.IsAutoReconnect());
    SetNodeExceptionReplyCallback(node.get());
  }
  return true;
}

void RedisClusterAsyncClient::HandleClusterSlotsReply(CDBClient* node, const CDBReply *cdb_reply)
{
  RedisClusterAsync* cluster = (RedisClusterAsync*)((RedisClient*)node)->GetCluster();
  if (cluster == NULL) return;

  const redisReply* reply = (const redisReply*)cdb_reply->GetReply();
  if (reply->type == REDIS_REPLY_ARRAY) {
    size_t cnt = reply->elements;
    for (size_t i = 0; i < cnt; i++) {
      auto elements = reply->element[i];
      if ( elements->type== REDIS_REPLY_ARRAY &&
          elements->elements >= 3 &&
          elements->element[0]->type == REDIS_REPLY_INTEGER &&
          elements->element[1]->type == REDIS_REPLY_INTEGER &&
          elements->element[2]->type == REDIS_REPLY_ARRAY &&
          elements->element[2]->elements >= 2 &&
          elements->element[2]->element[0]->type == REDIS_REPLY_STRING &&
          elements->element[2]->element[1]->type == REDIS_REPLY_INTEGER) {
        SlotRange slots = { elements->element[0]->integer, elements->element[1]->integer };
        string ip(elements->element[2]->element[0]->str);
        uint16_t port = elements->element[2]->element[1]->integer;

        auto new_node = cluster->AddNode(slots, ip, port, cluster->IsAutoReconnect());
        SetNodeExceptionReplyCallback(new_node.get());
        el_logger->debug("[RedisClusterAsyncClient::HandleClusterSlotsReply] slot range: ({}, {}), {}:{}",
                slots.first, slots.second, ip, port);
      }
    }
  }
}
void RedisClusterAsyncClient::SendAskRequest(RedisClient* node, const RedisRequestPtr& request, const string& ip, uint16_t port)
{
  el_logger->debug("[RedisClusterAsyncClient::SendAskRequest] Asking to node: {}:{}", ip, port);
  RedisClusterAsync* cluster = (RedisClusterAsync*)node->GetCluster();
  if (cluster) {
    auto target_node = cluster->GetNode(ip, port);
    if (!target_node) {
      target_node = cluster->AddNode(ip, port, cluster->IsAutoReconnect());
      SetNodeExceptionReplyCallback(target_node.get());
    }
    auto reply_cb = std::bind(&RedisClusterAsyncClient::HandleAskReply, this, std::placeholders::_1, std::placeholders::_2);
    target_node->SendCommand(reply_cb, "ASKING");
  }
}
void RedisClusterAsyncClient::SendRedirectRequest(RedisClient* node, const RedisRequestPtr& request, const string& ip, uint16_t port)
{
  el_logger->debug("[RedisClusterAsyncClient::SendRedirectRequest] Redirect to node: {}:{}", ip, port);
  RedisClusterAsync* cluster = (RedisClusterAsync*)node->GetCluster();
  if (cluster) {
    auto target_node = cluster->GetNode(ip, port);
    if (!target_node) {
      target_node = cluster->AddNode(ip, port, cluster->IsAutoReconnect());
      SetNodeExceptionReplyCallback(target_node.get());
    }
    target_node->SendCommand(request);
  }
}
void RedisClusterAsyncClient::HandleClusterDownReply(RedisClient* node, const RedisRequestPtr& request)
{
  el_logger->debug("[RedisClusterAsyncClient::HandleClusterDownReply] cmd: {}", request->ToString());
}
void RedisClusterAsyncClient::HandleAskReply(CDBClient* node, const CDBReply* cdb_reply)
{
  const redisReply* reply = (const redisReply*)cdb_reply->GetReply();
  el_logger->debug("[RedisClusterAsyncClient::HandleAskReply] reply status: {}", reply->str);
}
void RedisClusterAsyncClient::SetNodeExceptionReplyCallback(RedisAsyncClient* node)
{
  node->SetExcepitonReplyCallback(
      std::bind(&RedisClusterAsyncClient::SendAskRequest, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
      std::bind(&RedisClusterAsyncClient::SendRedirectRequest, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
      std::bind(&RedisClusterAsyncClient::HandleClusterDownReply, this, std::placeholders::_1, std::placeholders::_2)
      );
}

bool RedisClusterAsyncClient::SendCommand(CDBReply* msg, const char* format, ...)
{
  bool success = false;
  va_list ap;
  va_start(ap, format);
  string key = GetRedisKeyFromCommand(format, ap);
  RedisAsyncClientPtr node = cluster_.GetNode(key);
  if (node) {
    success = node->SendCommand(msg, format, ap);
  }
  va_end(ap);

  return success;
}

bool RedisClusterAsyncClient::SendCommand(const OnReplyCallback& reply_cb, const char* format, ...)
{
  bool success = false;
  va_list ap;
  va_start(ap, format);
  string key = GetRedisKeyFromCommand(format, ap);
  RedisAsyncClientPtr node = cluster_.GetNode(key);
  if (node) {
    success = node->SendCommand(reply_cb, format, ap);
  }
  va_end(ap);

  return success;
}

/*
 * Implements of RedisClusterClient
 */
bool RedisClusterSyncClient::Init(const IPAddressList& addr_list, const CDBCallbacksPtr& cdb_cbs, bool auto_reconnect)
{
  user_cdb_cbs_ = cdb_cbs;
  cluster_.SetAutoReconnect(auto_reconnect);
  SetClusterNodeCallbacks();

  for (const auto& addr : addr_list) {
    auto node = cluster_.AddNode(addr.ip_, addr.port_, cluster_.IsAutoReconnect());
    //SetNodeExceptionReplyCallback(node.get());
  }
  return true;
}

bool RedisClusterSyncClient::SendCommand(CDBReply* user_reply, const char* format, ...)
{
  bool success = false;
  va_list ap;
  va_start(ap, format);
  string key = GetRedisKeyFromCommand(format, ap);
  va_end(ap);

  va_list ap_copy;
  va_start(ap_copy, format);
  auto node = cluster_.GetNode(key);
  if (!node) {
    node = cluster_.GetAvailableNode();
  }
  if (node) {
    success = node->SendCommand(user_reply, format, ap_copy);
    if (!success) {
      el_logger->debug("[RedisClusterSyncClient::SendCommand] Send command failed");
    }
  } else {
    el_logger->debug("[RedisClusterSyncClient::SendCommand] Get node by key({}) failed and not has available node", key);
  }
  va_end(ap_copy);

  return success;
}

bool RedisClusterSyncClient::SendCommand(const OnReplyCallback& reply_cb, const char* format, ...)
{
  el_logger->debug("[RedisClusterSyncClient::SendCommand]: api not implemented");
  return false;
}

}  // namespace db_api

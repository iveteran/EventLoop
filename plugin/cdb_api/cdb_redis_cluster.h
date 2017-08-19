#ifndef _CDB_REDIS_CLUSTER_H
#define _CDB_REDIS_CLUSTER_H

#include "cdb_redis.h"
#include "redis_cluster.h"

namespace cdb_api {

class RedisClusterClient
{
  public:
  bool Init(const IPAddressList& addr_list, const CDBCallbacksPtr& cdb_cbs = nullptr, bool auto_reconnect = true);

  bool SendCommand(CDBReply* reply_msg, const char* format, ...);
  bool SendCommand(const OnReplyCallback& reply_cb, const char* format, ...);

  protected:

  void SetClusterNodeCallbacks();
  void OnNodeConnected(CDBClient* node);
  void OnNodeDisconnected(CDBClient* node);
  void OnNodeErrorOccurred(CDBClient* node, int errcode, const char* errstr);

  virtual void HandleClusterSlotsReply(CDBClient* node, const CDBReply *cdb_reply) { }
  virtual void SetConnectionEventsCallback(const CDBCallbacksPtr& cbs) = 0;

  protected:
  CDBCallbacksPtr   user_cdb_cbs_;
  uint32_t          connected_nodes_ = 0;
};

class RedisClusterAsyncClient : public RedisClusterClient
{
  public:
  bool Init(const IPAddressList& addr_list, const CDBCallbacksPtr& cdb_cbs = nullptr, bool auto_reconnect = true);

  bool SendCommand(CDBReply* reply_msg, const char* format, ...);
  bool SendCommand(const OnReplyCallback& reply_cb, const char* format, ...);

  void HandleClusterSlotsReply(CDBClient* node, const CDBReply *reply);
  void SendAskRequest(RedisClient* node, const RedisRequestPtr& request, const string& ip, uint16_t port);
  void SendRedirectRequest(RedisClient* node, const RedisRequestPtr& request, const string& ip, uint16_t port);
  void HandleClusterDownReply(RedisClient* node, const RedisRequestPtr& request);

  protected:
  void SetNodeExceptionReplyCallback(RedisAsyncClient* node);
  void SetConnectionEventsCallback(const CDBCallbacksPtr& cbs) { cluster_.SetCallbacks(cbs); }
  void HandleAskReply(CDBClient* node, const CDBReply*);

  private:
  RedisClusterAsync  cluster_;
};

class RedisClusterSyncClient : public RedisClusterClient
{
  public:
  bool Init(const IPAddressList& addr_list, const CDBCallbacksPtr& cdb_cbs = nullptr, bool auto_reconnect = true);

  bool SendCommand(CDBReply* reply_msg, const char* format, ...);
  bool SendCommand(const OnReplyCallback& reply_cb, const char* format, ...);

  protected:
  void SetConnectionEventsCallback(const CDBCallbacksPtr& cbs) { cluster_.SetCallbacks(cbs); }

  private:
  RedisClusterSync  cluster_;
};

}  // namespace cdb_api

#endif  // _CDB_REDIS_CLUSTER_H

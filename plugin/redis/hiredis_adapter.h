#ifndef _HIREDIS_ADAPTER_H
#define _HIREDIS_ADAPTER_H

#include "el.h"
#include "redis_callbacks.h"
#include "utils.h"
#include <async.h>
#include <hiredis.h>

using namespace evt_loop;
namespace hiredis {

class RedisMessage {
  public:
  RedisMessage(const redisReply* reply) : redis_reply_(reply) { }
  const redisReply* GetRedisReply() const { return redis_reply_; }

  private:
  const redisReply* redis_reply_;
};

class RedisAsyncClient : public IOEvent {
  public:
  RedisAsyncClient(const char* host, uint16_t port, bool auto_reconnect = true, RedisCallbacksPtr redis_cbs = nullptr);
  ~RedisAsyncClient();
  bool Init();

  bool Connect();
  void Disconnect();
  void SetRedisCallbacks(const RedisCallbacksPtr& redis_cbs);
  redisAsyncContext* RedisContext() { return redis_ctx_; }

  //bool SendCommand(const string& msg);
  bool SendCommand(const char* format, ...);

  void OnRedisReply(const redisAsyncContext* ctx, redisReply* reply);
  void OnRedisConnect(const redisAsyncContext* ctx, int status);
  void OnRedisDisconnect(const redisAsyncContext* ctx, int status);

  private:
  int SetContext(redisAsyncContext * ctx);
  bool Connect_();
  void Reconnect();
  //void SendTempBuffer();

  void OnEvents(uint32_t events);
  void OnError(int errcode, const char* errstr);
  void OnReconnectTimer(PeriodicTimer* timer);

  private:
  IPAddress           server_addr_;
  redisAsyncContext*  redis_ctx_;
  bool                auto_reconnect_;
  //list<string>        tmp_sendbuf_list_;
  PeriodicTimer       reconnect_timer_;

  RedisCallbacksPtr   redis_cbs_;
};

}  // ns hiredis

#endif /* !_HIREDIS_ADAPTER_H */

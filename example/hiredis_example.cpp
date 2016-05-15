#include <stdio.h>
#include <assert.h>
#include "el.h"
#include "cdb_redis.h"

#define TEST_STRING     "hiredis_async_test"
#define TEST_STRING_2   "hiredis_async_test 2"

using namespace evt_loop;
using namespace cdb_api;

class RedisClient_Test {
  public:
  RedisClient_Test()
  {
    m_client.Init("localhost", 6379);
    RedisMessage rmsg;
    bool success = false;
    const redisReply* reply = NULL;

    success = m_client.SendCommand(&rmsg, "SET mykey1 %s", TEST_STRING);
    assert(success);
    reply = (const redisReply*)rmsg.GetReply();
    printf("[RedisClient_Test] received reply: { type: %d, integer: %lld, len: %d, str: %s, elements: %lu, element list: %p }\n",
        reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);
    assert(reply->type == REDIS_REPLY_STATUS);
    assert(strcmp(reply->str, "OK") == 0);
    rmsg.ReleaseReplyObject();

    success = m_client.SendCommand(&rmsg, "HMSET myhash1 name yufangbin pwd 123456 status 0");
    assert(success);
    reply = (const redisReply*)rmsg.GetReply();
    printf("[RedisClient_Test] received reply: { type: %d, integer: %lld, len: %d, str: %s, elements: %lu, element list: %p }\n",
        reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);
    assert(reply->type == REDIS_REPLY_STATUS);
    assert(strcmp(reply->str, "OK") == 0);
    rmsg.ReleaseReplyObject();

    success = m_client.SendCommand(&rmsg, "HGETALL myhash1");
    assert(success);
    reply = (const redisReply*)rmsg.GetReply();
    printf("[RedisClient_Test] received reply: { type: %d, integer: %lld, len: %d, str: %s, elements: %lu, element list: %p }\n",
        reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);
    assert(reply->type == REDIS_REPLY_ARRAY);
    rmsg.ReleaseReplyObject();

    printf("--- All RedisClient tests passed!\n\n");
  }

  private:
  RedisClient m_client;
};

class RedisAsyncClient_Test {
    public:
    RedisAsyncClient_Test()
    {
      CDBCallbacksPtr cdb_cbs = std::make_shared<CDBCallbacks>();
      cdb_cbs->on_connected_cb = std::bind(&RedisAsyncClient_Test::OnConnectionCreated, this, std::placeholders::_1);
      m_client.Init("localhost", 6379, cdb_cbs);
      setkey_reply_cb_ = std::bind(&RedisAsyncClient_Test::SetkeyReplyCb, this, std::placeholders::_1, std::placeholders::_2);
      getkey_reply_cb_ = std::bind(&RedisAsyncClient_Test::GetkeyReplyCb, this, std::placeholders::_1, std::placeholders::_2);
      lrange_reply_cb_ = std::bind(&RedisAsyncClient_Test::LrangeReplyCb, this, std::placeholders::_1, std::placeholders::_2);
      hgetall_reply_cb_ = std::bind(&RedisAsyncClient_Test::HgetallReplyCb, this, std::placeholders::_1, std::placeholders::_2);
      exists_reply_cb_ = std::bind(&RedisAsyncClient_Test::ExistsReplyCb, this, std::placeholders::_1, std::placeholders::_2);
    }

    private:
    void SetkeyReplyCb(CDBClient* client, const CDBMessage* rmsg)
    {
        const redisReply* reply = (const redisReply*)rmsg->GetReply();
        printf("[RedisAsyncClient_Test::SetkeyReplyCb] received reply, fd: %d\n"
            " reply: { type: %d, integer: %lld, len: %d, str: %s, elements: %lu, element list: %p }\n",
            ((RedisAsyncClient *)client)->FD(), reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);

        assert(reply->type == REDIS_REPLY_STATUS);
        assert(strcmp(reply->str, "OK") == 0);
    }
    void GetkeyReplyCb(CDBClient* client, const CDBMessage* rmsg)
    {
        const redisReply* reply = (const redisReply*)rmsg->GetReply();
        printf("[RedisAsyncClient_Test::GetkeyReplyCb] received reply, fd: %d\n"
            " reply: { type: %d, integer: %lld, len: %d, str: %s, elements: %lu, element list: %p }\n",
            ((RedisAsyncClient *)client)->FD(), reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);

        assert(reply->type == REDIS_REPLY_STRING);
        assert(strcmp(reply->str, TEST_STRING) == 0);
    }
    void LrangeReplyCb(CDBClient* client, const CDBMessage* rmsg)
    {
        const redisReply* reply = (const redisReply*)rmsg->GetReply();
        printf("[RedisAsyncClient_Test::LrangeReplyCb] received reply, fd: %d\n"
            " reply: { type: %d, integer: %lld, len: %d, str: %s, elements: %lu, element list: %p }\n",
            ((RedisAsyncClient *)client)->FD(), reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);

        assert(reply->type == REDIS_REPLY_ARRAY);
    }
    void HgetallReplyCb(CDBClient* client, const CDBMessage* rmsg)
    {
        const redisReply* reply = (const redisReply*)rmsg->GetReply();
        printf("[RedisAsyncClient_Test::HgetallReplyCb] received reply, fd: %d\n"
            " reply: { type: %d, integer: %lld, len: %d, str: %s, elements: %lu, element list: %p }\n",
            ((RedisAsyncClient *)client)->FD(), reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);

        assert(reply->type == REDIS_REPLY_ARRAY);
    }
    void ExistsReplyCb(CDBClient* client, const CDBMessage* rmsg)
    {
        const redisReply* reply = (const redisReply*)rmsg->GetReply();
        printf("[RedisAsyncClient_Test::ExistsReplyCb] received reply, fd: %d\n"
            " reply: { type: %d, integer: %lld, len: %d, str: %s, elements: %lu, element list: %p }\n",
            ((RedisAsyncClient *)client)->FD(), reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);

        assert(reply->type == REDIS_REPLY_INTEGER);
        assert(reply->integer == 1);
        printf("--- All RedisAsyncClient tests passed!\n\n");
        EV_Singleton->StopLoop();
    }
    void OnConnectionCreated(CDBClient* client)
    {
        printf("[RedisAsyncClient_Test::OnConnectionCreated] connection created, fd: %d\n", ((RedisAsyncClient *)client)->FD());

        client->SendCommand(setkey_reply_cb_, "SET mykey %s", TEST_STRING);
        client->SendCommand(NULL, "SET mykey2 %s", TEST_STRING_2);
        client->SendCommand(NULL, "LPUSH mylist china guangdong shenzhen");
        client->SendCommand(NULL, "HMSET myhash username yufangbin password 123456 status 1");

        client->SendCommand(getkey_reply_cb_, "GET mykey");
        client->SendCommand(lrange_reply_cb_, "LRANGE mylist 0 -1");
        client->SendCommand(hgetall_reply_cb_, "HGETALL myhash");
        client->SendCommand(exists_reply_cb_, "EXISTS myhash");
    }

    private:
    RedisAsyncClient m_client;
    OnReplyCallback setkey_reply_cb_;
    OnReplyCallback getkey_reply_cb_;
    OnReplyCallback lrange_reply_cb_;
    OnReplyCallback hgetall_reply_cb_;
    OnReplyCallback exists_reply_cb_;
};

int main(int argc, char **argv) {
  RedisClient_Test hiredis_sync_test;
  RedisAsyncClient_Test hiredis_async_test;

  EV_Singleton->StartLoop();

  return 0;
}


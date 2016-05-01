#include <stdio.h>
#include "el.h"
#include "hiredis_adapter.h"

using namespace evt_loop;
using namespace hiredis;

class RedisAsyncClient_Test {
    public:
    RedisAsyncClient_Test() : m_client("localhost", 6379)
    {
      RedisCallbacksPtr redis_cbs = std::make_shared<RedisCallbacks>();
      redis_cbs->on_connected_cb = std::bind(&RedisAsyncClient_Test::OnConnectionCreated, this, std::placeholders::_1);
      redis_cbs->on_reply_cb = std::bind(&RedisAsyncClient_Test::OnMessageRecvd, this, std::placeholders::_1, std::placeholders::_2);
      m_client.SetRedisCallbacks(redis_cbs);
    }

    private:
    void OnMessageRecvd(RedisAsyncClient* client, const RedisMessage* rmsg)
    {
        const redisReply* reply = rmsg->GetRedisReply();
        printf("[RedisAsyncClient_Test::OnMessageRecvd] received reply, fd: %d\n"
            " reply: { type: %d, integer: %lld, len: %d, str: %s, elements: %lu, element list: %p }\n",
            client->FD(), reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);
    }
    void OnConnectionCreated(RedisAsyncClient* client)
    {
        printf("[RedisAsyncClient_Test::OnConnectionCreated] connection created, fd: %d\n", client->FD());

        printf("SET key\n");
        client->SendCommand("SET key %s", "hiredis_async_test 2");

        printf("GET key\n");
        client->SendCommand("GET key");
        client->SendCommand("LRANGE mylist 0 -1");
    }

    private:
    RedisAsyncClient m_client;
};

int main(int argc, char **argv) {
  RedisAsyncClient_Test hiredis_test;

  EV_Singleton->StartLoop();

  return 0;
}


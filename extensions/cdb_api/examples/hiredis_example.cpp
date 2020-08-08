#include <stdio.h>
#include <assert.h>
#include "el.h"
#include "cdb_redis.h"
#include "cdb_redis_cluster.h"

#define TEST_STRING     "hiredis_async_test"
#define TEST_STRING_2   "hiredis_async_test 2"

#define TEST_CLUSTER    0

using namespace evt_loop;
using namespace cdb_api;

class RedisClient_Test {
  public:
  RedisClient_Test()
  {
    CDBCallbacksPtr cdb_cbs = std::make_shared<CDBCallbacks>();
    cdb_cbs->on_connected_cb = std::bind(&RedisClient_Test::OnConnectionCreated, this, std::placeholders::_1);
#if TEST_CLUSTER
    IPAddressList addr_list{ IPAddress("localhost", 7000), IPAddress("127.0.0.1", 7003) };
    bool success = m_client.Init(addr_list, cdb_cbs);
#else
    bool success = m_client.Init("localhost", 6379, cdb_cbs);
#endif
    if (!success) {
      //printf("[RedisClient_Test] Init failed: %s\n", m_client.GetLastError());
    }
  }

  private:
  void OnConnectionCreated(CDBClient* client)
  {
    printf("[RedisClient_Test::OnConnectionCreated] connection created\n");

#if TEST_CLUSTER
    node_num++;
    if (node_num > 1) return;
#endif

    RedisReply rmsg;
    const redisReply* reply = NULL;
    bool success = false;

    success = m_client.SendCommand(&rmsg, "SET my_test_key1 %s", TEST_STRING);
    assert(success);
    reply = (const redisReply*)rmsg.GetReply();
    printf("[RedisClient_Test] received reply: { type: %d, integer: %lld, len: %ld, str: %s, elements: %lu, element list: %p }\n",
        reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);
    assert(reply->type == REDIS_REPLY_STATUS);
    assert(strcmp(reply->str, "OK") == 0);
    rmsg.ReleaseReplyObject();

    success = m_client.SendCommand(&rmsg, "HMSET my_test_hash1 name yufangbin pwd 123456 status 0");
    assert(success);
    reply = (const redisReply*)rmsg.GetReply();
    printf("[RedisClient_Test] received reply: { type: %d, integer: %lld, len: %ld, str: %s, elements: %lu, element list: %p }\n",
        reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);
    assert(reply->type == REDIS_REPLY_STATUS);
    assert(strcmp(reply->str, "OK") == 0);
    rmsg.ReleaseReplyObject();

    success = m_client.SendCommand(&rmsg, "HGETALL my_test_hash1");
    assert(success);
    reply = (const redisReply*)rmsg.GetReply();
    printf("[RedisClient_Test] received reply: { type: %d, integer: %lld, len: %ld, str: %s, elements: %lu, element list: %p }\n",
        reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);
    assert(reply->type == REDIS_REPLY_ARRAY);
    rmsg.ReleaseReplyObject();

    /// test for binary data
    char my_binary_data[] = {'x', 'y', 'z', 0x11, 0x00, 'a', 'b', 'c'};
    success = m_client.SendCommand(&rmsg, "set my_test_binary_data %b", &my_binary_data, sizeof(my_binary_data));
    assert(success);
    reply = (const redisReply*)rmsg.GetReply();
    printf("[RedisClient_Test] received reply: { type: %d, integer: %lld, len: %ld, str: %s, elements: %lu, element list: %p }\n",
        reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);
    assert(reply->type == REDIS_REPLY_STATUS);
    rmsg.ReleaseReplyObject();

    success = m_client.SendCommand(&rmsg, "get my_test_binary_data");
    assert(success);
    reply = (const redisReply*)rmsg.GetReply();
    printf("[RedisClient_Test] received reply: { type: %d, integer: %lld, len: %ld, str: %p, elements: %lu, element list: %p }\n",
        reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);
    assert(reply->type == REDIS_REPLY_STRING);
    rmsg.ReleaseReplyObject();
    // end test for binary data

    /// test for eval lua script
    success = m_client.SendCommand(&rmsg, "EVAL %s 0 %s", "return redis.call('del', unpack(redis.call('keys', ARGV[1])))", "my_test_*");
    assert(success);
    reply = (const redisReply*)rmsg.GetReply();
    printf("[RedisClient_Test] received reply: { type: %d, integer: %lld, len: %ld, str: %s, elements: %lu, element list: %p }\n",
        reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);
    assert(reply->type == REDIS_REPLY_INTEGER);
    rmsg.ReleaseReplyObject();
    // end test for eval lua script

    printf("--- All RedisSyncClient tests passed!\n\n");
  }

  private:
#if TEST_CLUSTER
  RedisClusterSyncClient m_client;
  int node_num = 0;
#else
  RedisSyncClient m_client;
#endif
};

class RedisAsyncClient_Test {
    public:
    RedisAsyncClient_Test()
    {
      CDBCallbacksPtr cdb_cbs = std::make_shared<CDBCallbacks>();
      cdb_cbs->on_connected_cb = std::bind(&RedisAsyncClient_Test::OnConnectionCreated, this, std::placeholders::_1);
      //cdb_cbs->on_closed_cb = std::bind(&RedisAsyncClient_Test::OnConnectionClosed, this, std::placeholders::_1);
      setkey_reply_cb_ = std::bind(&RedisAsyncClient_Test::SetkeyReplyCb, this, std::placeholders::_1, std::placeholders::_2);
      getkey_reply_cb_ = std::bind(&RedisAsyncClient_Test::GetkeyReplyCb, this, std::placeholders::_1, std::placeholders::_2);
      lrange_reply_cb_ = std::bind(&RedisAsyncClient_Test::LrangeReplyCb, this, std::placeholders::_1, std::placeholders::_2);
      hgetall_reply_cb_ = std::bind(&RedisAsyncClient_Test::HgetallReplyCb, this, std::placeholders::_1, std::placeholders::_2);
      exists_reply_cb_ = std::bind(&RedisAsyncClient_Test::ExistsReplyCb, this, std::placeholders::_1, std::placeholders::_2);
      set_binary_reply_cb_ = std::bind(&RedisAsyncClient_Test::SetBinaryDataCb, this, std::placeholders::_1, std::placeholders::_2);
      get_binary_reply_cb_ = std::bind(&RedisAsyncClient_Test::GetBinaryDataCb, this, std::placeholders::_1, std::placeholders::_2);
      eval_script_reply_cb_ = std::bind(&RedisAsyncClient_Test::EvalScriptCb, this, std::placeholders::_1, std::placeholders::_2);

#if TEST_CLUSTER
      IPAddressList addr_list{ IPAddress("localhost", 7000), IPAddress("127.0.0.1", 7003) };
      m_client.Init(addr_list, cdb_cbs);
#else
      m_client.Init("localhost", 6379, cdb_cbs);
#endif
    }

    private:
    void SetkeyReplyCb(CDBClient* client, const CDBReply* rmsg)
    {
        const redisReply* reply = (const redisReply*)rmsg->GetReply();
        printf("[RedisAsyncClient_Test::SetkeyReplyCb] received reply, fd: %d\n"
            " reply: { type: %d, integer: %lld, len: %ld, str: %s, elements: %lu, element list: %p }\n",
            ((RedisAsyncClient *)client)->FD(), reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);

        assert(reply->type == REDIS_REPLY_STATUS);
        assert(strcmp(reply->str, "OK") == 0);

        CheckTestFinished();
    }
    void GetkeyReplyCb(CDBClient* client, const CDBReply* rmsg)
    {
        const redisReply* reply = (const redisReply*)rmsg->GetReply();
        printf("[RedisAsyncClient_Test::GetkeyReplyCb] received reply, fd: %d\n"
            " reply: { type: %d, integer: %lld, len: %ld, str: %s, elements: %lu, element list: %p }\n",
            ((RedisAsyncClient *)client)->FD(), reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);

        assert(reply->type == REDIS_REPLY_STRING);
        assert(strcmp(reply->str, TEST_STRING) == 0);

        CheckTestFinished();
    }
    void LrangeReplyCb(CDBClient* client, const CDBReply* rmsg)
    {
        const redisReply* reply = (const redisReply*)rmsg->GetReply();
        printf("[RedisAsyncClient_Test::LrangeReplyCb] received reply, fd: %d\n"
            " reply: { type: %d, integer: %lld, len: %ld, str: %s, elements: %lu, element list: %p }\n",
            ((RedisAsyncClient *)client)->FD(), reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);

        assert(reply->type == REDIS_REPLY_ARRAY);

        CheckTestFinished();
    }
    void HgetallReplyCb(CDBClient* client, const CDBReply* rmsg)
    {
        const redisReply* reply = (const redisReply*)rmsg->GetReply();
        printf("[RedisAsyncClient_Test::HgetallReplyCb] received reply, fd: %d\n"
            " reply: { type: %d, integer: %lld, len: %ld, str: %s, elements: %lu, element list: %p }\n",
            ((RedisAsyncClient *)client)->FD(), reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);

        assert(reply->type == REDIS_REPLY_ARRAY);

        CheckTestFinished();
    }
    void ExistsReplyCb(CDBClient* client, const CDBReply* rmsg)
    {
        const redisReply* reply = (const redisReply*)rmsg->GetReply();
        printf("[RedisAsyncClient_Test::ExistsReplyCb] received reply, fd: %d\n"
            " reply: { type: %d, integer: %lld, len: %ld, str: %s, elements: %lu, element list: %p }\n",
            ((RedisAsyncClient *)client)->FD(), reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);

        assert(reply->type == REDIS_REPLY_INTEGER);
        assert(reply->integer == 1);

        CheckTestFinished();
    }
    void SetBinaryDataCb(CDBClient* client, const CDBReply* rmsg)
    {
        const redisReply* reply = (const redisReply*)rmsg->GetReply();
        printf("[RedisAsyncClient_Test::SetBinaryDataCb] received reply, fd: %d\n"
                " reply: { type: %d, integer: %lld, len: %ld, str: %s, elements: %lu, element list: %p }\n",
                ((RedisAsyncClient *)client)->FD(), reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);
        assert(reply->type == REDIS_REPLY_STATUS);

        CheckTestFinished();
    }
    void GetBinaryDataCb(CDBClient* client, const CDBReply* rmsg)
    {
        const redisReply* reply = (const redisReply*)rmsg->GetReply();
        printf("[RedisAsyncClient_Test::GetBinaryDataCb] received reply, fd: %d\n"
                " reply: { type: %d, integer: %lld, len: %ld, str: %p, elements: %lu, element list: %p }\n",
                ((RedisAsyncClient *)client)->FD(), reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);
        assert(reply->type == REDIS_REPLY_STRING);

        CheckTestFinished();
    }
    void EvalScriptCb(CDBClient* client, const CDBReply* rmsg)
    {
        const redisReply* reply = (const redisReply*)rmsg->GetReply();
        printf("[RedisClient_EvalScriptCb] received reply: { type: %d, integer: %lld, len: %ld, str: %s, elements: %lu, element list: %p }\n",
                reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);
        assert(reply->type == REDIS_REPLY_INTEGER);

        CheckTestFinished();
    }
    void CheckTestFinished()
    {
        rsp_num++;
        printf("req_num: %d, rsp_num: %d\n", req_num, rsp_num);
        if (req_num == rsp_num) {
          printf("--- All RedisAsyncClient tests passed!\n\n");
          //EV_Singleton->StopLoop();
        }
    }
    void OnConnectionCreated(CDBClient* client)
    {
        printf("[RedisAsyncClient_Test::OnConnectionCreated] connection created, fd: %d\n", ((RedisAsyncClient *)client)->FD());
#if TEST_CLUSTER
        printf("[RedisAsyncClient_Test::OnConnectionCreated] node_num: %d\n", node_num);
        node_num++;
        if (node_num > 1) return;
#endif
        req_num = 0;
        rsp_num = 0;

        bool success = false;
        success = client->SendCommand(setkey_reply_cb_, "SET my_test_key %s", TEST_STRING); req_num++;
        assert(success);
        success = client->SendCommand(NULL, "SET my_test_key2 %s", TEST_STRING_2); //req_num++;
        assert(success);
        const char* str_1 = "china";
        const char* str_2 = "guangdong";
        const char* str_3 = "shenzhen";
        success = client->SendCommand(NULL, "LPUSH my_test_list %s %s %s", str_1, str_2, str_3); //req_num++;
        assert(success);
        const char* username = "yufangbin";
        const char* password = "abc123";
        int status = 1;
        success = client->SendCommand(NULL, "HMSET my_test_hash username %s password %s status %d", username, password, status); //req_num++;
        assert(success);

        success = client->SendCommand(getkey_reply_cb_, "GET my_test_key"); req_num++;
        assert(success);
        success = client->SendCommand(lrange_reply_cb_, "LRANGE my_test_list %d %d", 0, -1); req_num++;
        assert(success);
        success = client->SendCommand(hgetall_reply_cb_, "HGETALL my_test_hash"); req_num++;
        assert(success);
        success = client->SendCommand(exists_reply_cb_, "EXISTS my_test_hash"); req_num++;
        assert(success);

        char my_binary_data[] = {'x', 'y', 'z', 0x11, 0x00, 'a', 'b', 'c'};
        success = client->SendCommand(set_binary_reply_cb_, "set my_test_binary_data %b", &my_binary_data, sizeof(my_binary_data)); req_num++;
        assert(success);

        success = client->SendCommand(get_binary_reply_cb_, "get my_test_binary_data"); req_num++;
        assert(success);

        success = m_client.SendCommand(eval_script_reply_cb_, "EVAL %s 0 %s", "return redis.call('del', unpack(redis.call('keys', ARGV[1])))", "my_test_*"); req_num++;
        assert(success);
    }
    void OnConnectionClosed(CDBClient* client)
    {
        printf("[RedisAsyncClient_Test::OnConnectionClosed] connection closed, fd: %d\n", ((RedisAsyncClient *)client)->FD());
#if TEST_CLUSTER
        node_num = 0;
#endif
    }

    private:
#if TEST_CLUSTER
    RedisClusterAsyncClient m_client;
    int node_num = 0;
#else
    RedisAsyncClient m_client;
#endif
    OnReplyCallback setkey_reply_cb_;
    OnReplyCallback getkey_reply_cb_;
    OnReplyCallback lrange_reply_cb_;
    OnReplyCallback hgetall_reply_cb_;
    OnReplyCallback exists_reply_cb_;
    OnReplyCallback set_binary_reply_cb_;
    OnReplyCallback get_binary_reply_cb_;
    OnReplyCallback eval_script_reply_cb_;
    int req_num = 0;
    int rsp_num = 0;
};

int main(int argc, char **argv) {
  RedisClient_Test hiredis_sync_test;
  RedisAsyncClient_Test hiredis_async_test;

  EV_Singleton->StartLoop();

  return 0;
}


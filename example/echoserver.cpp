#include <stdio.h>
#include "eventloop/el.h"

using namespace evt_loop;

//#define TEST_BINARY_MESSAGE
//#define TEST_BINARY_MESSAGE_CLIENT

#define TEST_CRLF_MESSAGE
//#define TEST_CRLF_MESSAGE_CLIENT

//#define TEST_JSON_MESSAGE
//#define TEST_JSON_MESSAGE_CLIENT

#define TEST_CRLF_MESSAGE_IPV6

class BusinessTester {
    public:
    BusinessTester() : dummy_(0)
#ifdef TEST_BINARY_MESSAGE
      , echoserver_binary_(IPVer::V4, "0.0.0.0", 10000, MessageType::BINARY)
  #ifdef TEST_BINARY_MESSAGE_CLIENT
      , echoclient_binary_(IPVer::V4, "localhost", 10000, MessageType::BINARY)
  #endif
      , echoserver_binary2_(IPVer::V4, "0.0.0.0", 20000, MessageType::BINARY)
#endif
#ifdef TEST_CRLF_MESSAGE
      , echoserver_crlf_(IPVer::V4, "0.0.0.0", 10001, MessageType::CRLF)
  #ifdef TEST_CRLF_MESSAGE_CLIENT
      , echoclient_crlf_(IPVer::V4, "localhost", 10001, MessageType::CRLF)
  #endif
#endif
#ifdef TEST_JSON_MESSAGE
      , echoserver_json_(IPVer::V4, "0.0.0.0", 10002, MessageType::JSON)
  #ifdef TEST_JSON_MESSAGE_CLIENT
      , echoclient_json_(IPVer::V4, "localhost", 10002, MessageType::JSON)
  #endif
#endif
#ifdef TEST_CRLF_MESSAGE_IPV6
      , echoserver_ip6_(IPVer::V6_ONLY, "::", 30000, MessageType::CRLF)
#endif
    {
        TcpCallbacksPtr echo_svr_1_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        echo_svr_1_cbs->on_conn_ready_cb = std::bind(&BusinessTester::OnNewConnection, this, std::placeholders::_1);
        echo_svr_1_cbs->on_msg_recvd_cb = std::bind(&BusinessTester::OnMessageRecvd_1, this, std::placeholders::_1, std::placeholders::_2);

        TcpCallbacksPtr echo_client_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        echo_client_cbs->on_msg_recvd_cb = std::bind(&BusinessTester::OnMessageRecvd_Client, this, std::placeholders::_1, std::placeholders::_2);
        echo_client_cbs->on_conn_ready_cb = std::bind(&BusinessTester::OnConnectionReady, this, std::placeholders::_1);

        bool success = false;
#ifdef TEST_BINARY_MESSAGE
        echoserver_binary_.SetTcpCallbacks(echo_svr_1_cbs);
        echoserver_binary_.EnableIdleTimeout(10, std::bind(&BusinessTester::OnConnectionIdleTimeout, this, std::placeholders::_1, std::placeholders::_2));
        success = echoserver_binary_.Start();
        assert(success);

  #ifdef TEST_BINARY_MESSAGE_CLIENT
        echoclient_binary_.SetTcpCallbacks(echo_client_cbs);
        echoclient_binary_.EnableIdleTimeout(15, std::bind(&BusinessTester::OnConnectionIdleTimeout, this, std::placeholders::_1, std::placeholders::_2));
        echoclient_binary_.EnableHeartbeat(8, 1, 3);
        success = echoclient_binary_.Connect();
        assert(success);

        echoclient_binary_.Send("hello, binary message");
        echoclient_binary_.Send("hello china");
  #endif

        TcpCallbacksPtr echo_svr_2_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        echo_svr_2_cbs->on_msg_recvd_cb = std::bind(&BusinessTester::OnMessageRecvd_2, this, std::placeholders::_1, std::placeholders::_2);
        echoserver_binary2_.SetTcpCallbacks(echo_svr_2_cbs);
        echoserver_binary2_.EnableHeartbeat();
        success = echoserver_binary2_.Start();
        assert(success);
#endif

#ifdef TEST_CRLF_MESSAGE
        echoserver_crlf_.SetTcpCallbacks(echo_svr_1_cbs);
        success = echoserver_crlf_.Start();
        assert(success);

  #ifdef TEST_CRLF_MESSAGE_CLIENT
        echoclient_crlf_.SetTcpCallbacks(echo_client_cbs);
        echoclient_crlf_.EnableHeartbeat();
        success = echoclient_crlf_.Connect();
        assert(success);

        echoclient_crlf_.Send("hello, crlf message\r\n");
        echoclient_crlf_.Send("hello, shenzhen\r\n");
  #endif
#endif

#ifdef TEST_JSON_MESSAGE
        echoserver_json_.SetTcpCallbacks(echo_svr_1_cbs);
        success = echoserver_json_.Start();
        assert(success);

  #ifdef TEST_JSON_MESSAGE_CLIENT
        echoclient_json_.SetTcpCallbacks(echo_client_cbs);
        echoclient_json_.EnableHeartbeat();
        success = echoclient_json_.Connect();
        assert(success);

        echoclient_json_.Send(R"({ "name" : "yufangbin", "say" : "hello" })");
        echoclient_json_.Send(R"({
            "name" : "myobject",
            "type" : "int",
            "value" : 123,
            "child" : {
                "name" : "sub_obj",
                "type" : "kv",
                "value" : {
                    "type" : "float",
                    "value" : 3.123
                }
            },
            "brother" : {
                "name" : "bro_obj",
                "type" : "string",
                "value" : "bob"
            }
        })");
  #endif
#endif

#ifdef TEST_CRLF_MESSAGE_IPV6
        TcpCallbacksPtr echo_svr_ip6_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        echo_svr_ip6_cbs->on_msg_recvd_cb = std::bind(&BusinessTester::OnMessageRecvd_ip6, this, std::placeholders::_1, std::placeholders::_2);
        echoserver_ip6_.SetTcpCallbacks(echo_svr_ip6_cbs);
        echoserver_ip6_.SetNewClientCallback(std::bind(&BusinessTester::OnNewConnection_ip6, this, std::placeholders::_1));
        success = echoserver_ip6_.Start();
        assert(success);
#endif
    }
    void OnSignal(SignalHandler* sh, uint32_t signo)
    {
        printf("Shutdown\n");
        EV_Singleton->StopLoop();
    }

    private:
    void OnNewConnection(TcpConnection* conn)
    {
        printf("[OnNewConnection] fd: %d\n", conn->FD());
    }
    void OnConnectionReady(TcpConnection* conn)
    {
        printf("[OnConnectionReady] fd: %d\n", conn->FD());
        int msg_type = conn->GetMessageType();
        switch (msg_type) {
          case MessageType::BINARY:
            conn->Send("hello china 2");
            break;
          case MessageType::CRLF:
            conn->Send("hello china 2\r\n");
            break;
          case MessageType::JSON:
            conn->Send(R"({"name": "hello china 2"})");
            break;
          default:
            break;
        }
    }
    void OnConnectionIdleTimeout(TcpConnection* conn, uint32_t time)
    {
        printf("[OnConnectionIdleTimeout] fd: %d, now: %ld\n", conn->FD(), Now());
        //conn->Disconnect();
    }
    void OnMessageRecvd_1(TcpConnection* conn, const Message* msg)
    {
        printf("[echoserver1] fd: %d, message: %s, length: %lu\n", conn->FD(), msg->Payload(), msg->PayloadSize());
        cout << msg->DumpHexWithChars() << endl;
        //conn->Send(msg->Payload(), msg->PayloadSize());
        conn->Send(*msg);
    }
    void OnMessageRecvd_2(TcpConnection* conn, const Message* msg)
    {
        printf("[echoserver2] fd: %d, message: %s, length: %lu\n", conn->FD(), msg->Payload(), msg->PayloadSize());
        if (!strncmp(msg->Payload(), "ping", msg->PayloadSize()))
          conn->Send("pong");
        else
          conn->Send(msg->Payload(), msg->PayloadSize());
          //conn->Send(*msg);
    }
    void OnMessageRecvd_ip6(TcpConnection* conn, const Message* msg)
    {
        printf("[echoserver_ip6] fd: %d, message: %s, length: %lu\n", conn->FD(), msg->Payload(), msg->PayloadSize());
        //conn->Send(msg->Payload(), msg->PayloadSize());
        conn->Send(*msg);
    }
    void OnNewConnection_ip6(TcpConnection* conn)
    {
        printf("[OnNewConnection_ip6] fd: %d\n", conn->FD());
    }
    void OnMessageRecvd_Client(TcpConnection* conn, const Message* msg)
    {
        printf("[echoclient] fd: %d, message: %s, length: %lu\n", conn->FD(), msg->Payload(), msg->PayloadSize());
        cout << msg->DumpHexWithChars() << endl;
    }

    private:
    int dummy_;
#ifdef TEST_BINARY_MESSAGE
    TcpServer echoserver_binary_;
  #ifdef TEST_BINARY_MESSAGE_CLIENT
    TcpClient echoclient_binary_;
  #endif
    TcpServer echoserver_binary2_;
#endif

#ifdef TEST_CRLF_MESSAGE
    TcpServer echoserver_crlf_;
  #ifdef TEST_CRLF_MESSAGE_CLIENT
    TcpClient echoclient_crlf_;
  #endif
#endif

#ifdef TEST_JSON_MESSAGE
    TcpServer echoserver_json_;
  #ifdef TEST_JSON_MESSAGE
    TcpClient echoclient_json_;
  #endif
#endif

#ifdef TEST_CRLF_MESSAGE_IPV6
    TcpServer echoserver_ip6_;
#endif
};

int main(int argc, char **argv) {
  BusinessTester biz_tester;
  SignalHandler sh(SignalEvent::INT, std::bind(&BusinessTester::OnSignal, &biz_tester, std::placeholders::_1, std::placeholders::_2));

  EV_Singleton->StartLoop();

  return 0;
}

// How to usage telnet or nc to connect the server:
//  1) use telnet 0 10001 or nc -4 -C 0 10001 to connect and test echo server with CRLF message
//  2) use nc -6 -C ::1 30000 for IPv6 with CRLF message

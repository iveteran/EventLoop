#include <stdio.h>

#include "el.h"

namespace evt_loop {

class BusinessTester {
    public:
    BusinessTester() :
      echoserver_binary_("0.0.0.0", 10000, MessageType::BINARY),
      echoclient_binary_("localhost", 10000, MessageType::BINARY),
      echoserver_crlf_("0.0.0.0", 10001, MessageType::CRLF),
      echoclient_crlf_("localhost", 10001, MessageType::CRLF),
      echoserver_json_("0.0.0.0", 10002, MessageType::JSON),
      echoclient_json_("localhost", 10002, MessageType::JSON),
      echoserver_binary2_("0.0.0.0", 20000, MessageType::BINARY)
    {
        TcpCallbacksPtr echo_svr_1_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        echo_svr_1_cbs->on_msg_recvd_cb = std::bind(&BusinessTester::OnMessageRecvd_1, this, std::placeholders::_1, std::placeholders::_2);

        TcpCallbacksPtr echo_client_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        echo_client_cbs->on_msg_recvd_cb = std::bind(&BusinessTester::OnMessageRecvd_Client, this, std::placeholders::_1, std::placeholders::_2);

        echoserver_binary_.SetTcpCallbacks(echo_svr_1_cbs);
        echoclient_binary_.SetTcpCallbacks(echo_client_cbs);

        echoserver_crlf_.SetTcpCallbacks(echo_svr_1_cbs);
        echoclient_crlf_.SetTcpCallbacks(echo_client_cbs);

        echoserver_json_.SetTcpCallbacks(echo_svr_1_cbs);
        echoclient_json_.SetTcpCallbacks(echo_client_cbs);

        echoclient_binary_.Send("hello, binary message");
        echoclient_binary_.Send("hello china");

        echoclient_crlf_.Send("hello, crlf message\r\n");
        echoclient_crlf_.Send("hello, shenzhen\r\n");

        echoclient_json_.Send("{ name : yufangbin, say : hello }");
        echoclient_json_.Send("{ name : myobject, \r\n"
            " type : int, \r\n"
            " value : 123, \r\n"
            " child : {name : sub_obj, type : kv, value : {type : float, value : 3.123} }, \r\n"
            " brother : { name : bro_obj, type : string, value : bob} }");

        TcpCallbacksPtr echo_svr_2_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        echo_svr_2_cbs->on_msg_recvd_cb = std::bind(&BusinessTester::OnMessageRecvd_2, this, std::placeholders::_1, std::placeholders::_2);
        echoserver_binary2_.SetTcpCallbacks(echo_svr_2_cbs);
    }
    void OnSignal(SignalHandler* sh, uint32_t signo)
    {
        printf("Shutdown\n");
        EV_Singleton->StopLoop();
    }

    private:
    void OnMessageRecvd_1(TcpConnection* conn, const Message* msg)
    {
        printf("[echoserver1] fd: %d, message: %s, length: %lu\n", conn->FD(), msg->Payload(), msg->PayloadSize());
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
    void OnMessageRecvd_Client(TcpConnection* conn, const Message* msg)
    {
        printf("[echoclient] fd: %d, message: %s, length: %lu\n", conn->FD(), msg->Payload(), msg->PayloadSize());
    }

    private:
    TcpServer echoserver_binary_;
    TcpClient echoclient_binary_;
    TcpServer echoserver_crlf_;
    TcpClient echoclient_crlf_;
    TcpServer echoserver_json_;
    TcpClient echoclient_json_;
    TcpServer echoserver_binary2_;
};

}  // ns evt_loop

using namespace evt_loop;

int main(int argc, char **argv) {
  BusinessTester biz_tester;
  SignalHandler sh(SignalEvent::INT, std::bind(&BusinessTester::OnSignal, &biz_tester, std::placeholders::_1, std::placeholders::_2));

  EV_Singleton->StartLoop();

  return 0;
}


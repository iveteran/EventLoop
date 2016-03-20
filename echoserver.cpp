#include <stdio.h>

#include "tcp_server.h"
#include "tcp_client.h"

namespace evt_loop {

class BusinessTester {
    public:
    BusinessTester() : echoserver_("0.0.0.0", 22222), echoserver2_("0.0.0.0", 22223), echoclient_("localhost", 22222)
    {
        TcpCallbacksPtr echo_svr_1_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        echo_svr_1_cbs->on_msg_recvd_cb = std::bind(&BusinessTester::OnMessageRecvd_1, this, std::placeholders::_1, std::placeholders::_2);
        echoserver_.SetTcpCallbacks(echo_svr_1_cbs);

        TcpCallbacksPtr echo_svr_2_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        echo_svr_2_cbs->on_msg_recvd_cb = std::bind(&BusinessTester::OnMessageRecvd_2, this, std::placeholders::_1, std::placeholders::_2);
        echoserver2_.SetTcpCallbacks(echo_svr_2_cbs);

        TcpCallbacksPtr echo_client_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        echo_client_cbs->on_msg_recvd_cb = std::bind(&BusinessTester::OnMessageRecvd_3, this, std::placeholders::_1, std::placeholders::_2);
        echoclient_.SetTcpCallbacks(echo_client_cbs);

        echoclient_.Send("hello");
        echoclient_.Send("hello china");
    }

    private:
    void OnMessageRecvd_1(TcpConnection* conn, const Message* msg)
    {
        printf("[echoserver1] fd: %d, message: %s, length: %d\n", conn->FD(), msg->Payload(), msg->PayloadSize());
        //conn->Send(msg->Payload(), msg->PayloadSize());
        conn->Send(*msg);
    }
    void OnMessageRecvd_2(TcpConnection* conn, const Message* msg)
    {
        printf("[echoserver2] fd: %d, message: %s, length: %d\n", conn->FD(), msg->Payload(), msg->PayloadSize());
        if (!strncmp(msg->Payload(), "ping", msg->PayloadSize()))
          conn->Send("pong");
        else
          conn->Send(msg->Payload(), msg->PayloadSize());
          //conn->Send(*msg);
    }
    void OnMessageRecvd_3(TcpConnection* conn, const Message* msg)
    {
        printf("[echoclient] fd: %d, message: %s, length: %d\n", conn->FD(), msg->Payload(), msg->PayloadSize());
    }

    private:
    TcpServer echoserver_;
    TcpServer echoserver2_;
    TcpClient echoclient_;
};

class SignalHandler : public SignalEvent {
 public:
  SignalHandler()
  {
      SetSignal(SignalEvent::INT);
      EV_Singleton->AddEvent(this);
  }

 protected:
  void OnEvents(uint32_t events) {
    printf("shutdown\n");
    EV_Singleton->StopLoop();
  }
};

}  // ns evt_loop

using namespace evt_loop;

int main(int argc, char **argv) {
  BusinessTester biz_tester;
  SignalHandler s;

  EV_Singleton->StartLoop();

  return 0;
}


#include <stdio.h>
#include <tuple>

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
    }

    private:
    void OnMessageRecvd_1(TcpConnection* conn, const string* msg)
    {
        printf("[echoserver1] fd: %d, message: %s\n", conn->FD(), msg->c_str());
        conn->Send(*msg);
    }
    void OnMessageRecvd_2(TcpConnection* conn, const string* msg)
    {
        printf("[echoserver2] fd: %d, message: %s\n", conn->FD(), msg->c_str());
        conn->Send(*msg);
    }
    void OnMessageRecvd_3(TcpConnection* conn, const string* msg)
    {
        printf("[echoclient] fd: %d, message: %s\n", conn->FD(), msg->c_str());
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


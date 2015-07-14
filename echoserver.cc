#include <stdio.h>
#include <tuple>

#include "tcp_server.h"
#include "tcp_client.h"

namespace richinfo {

class BusinessTester {
    public:
    BusinessTester() : echoserver_("0.0.0.0", 22222), echoclient_("localhost", 22222)
    {
        echoserver_.SetOnMsgRecvdCb(OnMsgRecvdCallback(this, &BusinessTester::EchoServerMsgHandler2));
        echoclient_.SetOnMsgRecvdCb(OnMsgRecvdCallback(this, &BusinessTester::EchoClientMsgHandler2));

        echoclient_.Send("hello");
    }

    protected:
    void EchoServerMsgHandler(std::tuple<TcpConnection*, const string*> conn_msg_tuple)
    {
        TcpConnection* conn = std::get<0>(conn_msg_tuple);
        const string* msg = std::get<1>(conn_msg_tuple);

        printf("[echoserver] fd: %d, message: %s\n", conn->FD(), msg->c_str());
        conn->Send(*msg);
    }
    void EchoClientMsgHandler(std::tuple<TcpConnection*, const string*> conn_msg_tuple)
    {
        TcpConnection* conn = std::get<0>(conn_msg_tuple);
        const string* msg = std::get<1>(conn_msg_tuple);

        printf("[echoclient] fd: %d, message: %s\n", conn->FD(), msg->c_str());
    }
    void EchoServerMsgHandler2(TcpConnection* conn, const string* msg)
    {
        printf("[echoserver] fd: %d, message: %s\n", conn->FD(), msg->c_str());
        conn->Send(*msg);
    }
    void EchoClientMsgHandler2(TcpConnection* conn, const string* msg)
    {
        printf("[echoclient] fd: %d, message: %s\n", conn->FD(), msg->c_str());
    }

    private:
    TcpServer echoserver_;
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

}  // ns richinfo

using namespace richinfo;

int main(int argc, char **argv) {
  BusinessTester biz_tester;
  SignalHandler s;

  EV_Singleton->StartLoop();

  return 0;
}


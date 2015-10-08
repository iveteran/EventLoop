#include <stdio.h>
#include <tuple>

#include "tcp_server.h"
#include "tcp_client.h"

namespace evt_loop {

class EchoServer1_DataHandler : public ITcpEventHandler {
    public:
    void OnMessageRecvd(TcpConnection* conn, const string* msg)
    {
        printf("[echoserver1] fd: %d, message: %s\n", conn->FD(), msg->c_str());
        conn->Send(*msg);
    }
};

class EchoServer2_DataHandler : public ITcpEventHandler {
    public:
    void OnMessageRecvd(TcpConnection* conn, const string* msg)
    {
        printf("[echoserver2] fd: %d, message: %s\n", conn->FD(), msg->c_str());
        conn->Send(*msg);
    }
};

class EchoClient_DataHandler : public ITcpEventHandler {
    public:
    void OnMessageRecvd(TcpConnection* conn, const string* msg)
    {
        printf("[echoclient] fd: %d, message: %s\n", conn->FD(), msg->c_str());
    }
};

class BusinessTester {
    public:
    BusinessTester() : echoserver_("0.0.0.0", 22222), echoserver2_("0.0.0.0", 22223), echoclient_("localhost", 22222)
    {
        echoserver_.SetTcpEventHandler(&echo_svr_1_data_handler);
        echoserver2_.SetTcpEventHandler(&echo_svr_2_data_handler);
        echoclient_.SetTcpEventHandler(&echo_client_data_handler);

        echoclient_.Send("hello");
    }

    private:
    TcpServer echoserver_;
    EchoServer1_DataHandler echo_svr_1_data_handler;

    TcpServer echoserver2_;
    EchoServer2_DataHandler echo_svr_2_data_handler;

    TcpClient echoclient_;
    EchoClient_DataHandler echo_client_data_handler;
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


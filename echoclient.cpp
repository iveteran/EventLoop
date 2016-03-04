#include <stdio.h>
#include <tuple>

#include "tcp_client.h"

namespace evt_loop {

class BusinessTester {
    public:
    BusinessTester() : echoclient_("localhost", 22223)
    {
        TcpCallbacks *echo_client_cbs = new TcpCallbacks;
        echo_client_cbs->on_msg_recvd_cb = std::bind(&BusinessTester::OnMessageRecvd, this, std::placeholders::_1, std::placeholders::_2);
        echoclient_.SetTcpCallbacks(echo_client_cbs);

        echoclient_.Send("hello");
    }

    protected:
    void OnMessageRecvd(TcpConnection* conn, const string* msg)
    {
        printf("[echoclient] fd: %d, message: %s\n", conn->FD(), msg->c_str());
    }

    private:
    TcpClient echoclient_;
};
}   // ns evt_loop

using namespace evt_loop;

int main(int argc, char **argv) {
  BusinessTester biz_tester;

  EV_Singleton->StartLoop();

  return 0;
}


#include <stdio.h>
#include <tuple>

#include "tcp_client.h"

namespace richinfo {

class BusinessTester : public ITcpEventHandler {
    public:
    BusinessTester() : echoclient_("localhost", 22223)
    {
        echoclient_.SetTcpEventHandler(this);

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
}   // ns richinfo

using namespace richinfo;

int main(int argc, char **argv) {
  BusinessTester biz_tester;

  EV_Singleton->StartLoop();

  return 0;
}


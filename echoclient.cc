#include <stdio.h>
#include <tuple>

#include "tcp_client.h"

namespace richinfo {

class BusinessTester {
    public:
    BusinessTester() : echoclient_("localhost", 22222)
    {
        echoclient_.SetOnMessageCb(OnMessageCallback(this, &BusinessTester::EchoClientMsgHandler2));

        echoclient_.Send("hello");
    }

    protected:
    void EchoClientMsgHandler(std::tuple<TcpConnection*, const string*> conn_msg_tuple)
    {
        TcpConnection* conn = std::get<0>(conn_msg_tuple);
        const string* msg = std::get<1>(conn_msg_tuple);

        printf("[echoclient] fd: %d, message: %s\n", conn->FD(), msg->c_str());
    }
    void EchoClientMsgHandler2(TcpConnection* conn, const string* msg)
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


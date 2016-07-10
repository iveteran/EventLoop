#include <stdio.h>
#include "el.h"

namespace evt_loop {

class BusinessTester {
    public:
    BusinessTester() : echoclient_("localhost", 20000, MessageType::BINARY),
        sending_timer_(std::bind(&BusinessTester::OnSendingTimer, this, std::placeholders::_1))
    {
        TcpCallbacksPtr echo_client_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        echo_client_cbs->on_msg_recvd_cb = std::bind(&BusinessTester::OnMessageRecvd, this, std::placeholders::_1, std::placeholders::_2);

        echoclient_.SetNewClientCallback(std::bind(&BusinessTester::OnConnectionCreated, this, std::placeholders::_1));
        echoclient_.SetTcpCallbacks(echo_client_cbs);
        echoclient_.EnableKeepAlive(true);

        echoclient_.Connect();
        echoclient_.Send("hello world");

        TimeVal tv(10, 0);
        sending_timer_.SetInterval(tv);
    }

    protected:
    void OnMessageRecvd(TcpConnection* conn, const Message* msg)
    {
        printf("[echoclient] received message, fd: %d, message: %s, length: %lu\n", conn->FD(), msg->Payload(), msg->PayloadSize());
        if (!strncmp(msg->Payload(), "reconnect request", msg->PayloadSize()))
        {
            EV_Singleton->StopLoop();
        }
    }
    void OnConnectionCreated(TcpConnection* conn)
    {
        printf("[echoclient] connection created, fd: %d\n", conn->FD());
        printf("[OnConnectionCreated] ping\n");
        conn->Send("ping");
        sending_timer_.Start();
    }

    void OnSendingTimer(PeriodicTimer* timer)
    {
        printf("[OnSendingTimer] ping\n");
        echoclient_.Send("ping");
    }

    private:
    TcpClient echoclient_;
    PeriodicTimer sending_timer_;
};
}   // ns evt_loop

using namespace evt_loop;

int main(int argc, char **argv) {
  BusinessTester biz_tester;

  EV_Singleton->StartLoop();

  return 0;
}


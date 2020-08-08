#include <stdio.h>
#include "el.h"
#include "tls_client.h"

namespace evt_loop {

class BusinessTester {
    public:
    BusinessTester() :
        echoclient_("localhost", 50000, MessageType::BINARY),
        sending_timer_(TimeVal(5, 0), std::bind(&BusinessTester::OnSendingTimer, this, std::placeholders::_1))
    {
        //TLSConnection::setSSLCertKey("./ca/certs/device.cert.pem", "./ca/private/device.key.pem", "./ca/certs/ca.cert.pem");
        //TLSConnection::setSSLCertKey("server.pem", "server.pem");
        TcpCallbacksPtr echo_client_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        echo_client_cbs->on_msg_recvd_cb = std::bind(&BusinessTester::OnMessageRecvd, this, std::placeholders::_1, std::placeholders::_2);
        echo_client_cbs->on_conn_ready_cb = std::bind(&BusinessTester::OnConnectionReady, this, std::placeholders::_1);

        echoclient_.SetTcpCallbacks(echo_client_cbs);
        echoclient_.EnableKeepAlive(true);

        echoclient_.Connect();
        echoclient_.Send("hello world");
    }

    void OnSignal(SignalHandler* sh, uint32_t signo)
    {
        printf("Shutdown\n");
        EV_Singleton->StopLoop();
    }

    protected:
    void OnMessageRecvd(TcpConnection* conn, const Message* msg)
    {
        printf("[OnMessageRecvd] received message, fd: %d, message: %s, length: %lu\n", conn->FD(), msg->Payload(), msg->PayloadSize());
        if (!strncmp(msg->Payload(), "reconnect request", msg->PayloadSize()))
        {
            EV_Singleton->StopLoop();
        }
    }
    void OnConnectionReady(TcpConnection* conn)
    {
        printf("[OnConnectionReady] connection ready, fd: %d\n", conn->FD());
        printf("[OnConnectionReady] ping\n");
        conn->Send("ping");
        sending_timer_.Start();
    }

    void OnSendingTimer(TimerEvent* timer)
    {
        printf("[OnSendingTimer] ping\n");
        echoclient_.Send("ping");
    }

    private:
    TLSClient echoclient_;
    PeriodicTimer sending_timer_;
};
}   // ns evt_loop

using namespace evt_loop;

int main(int argc, char **argv) {
  BusinessTester biz_tester;
  SignalHandler sh(SignalEvent::INT, std::bind(&BusinessTester::OnSignal, &biz_tester, std::placeholders::_1, std::placeholders::_2));

  EV_Singleton->StartLoop();

  return 0;
}


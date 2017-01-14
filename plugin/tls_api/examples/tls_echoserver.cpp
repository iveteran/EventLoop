#include <stdio.h>

#include "el.h"
#include "tls_server.h"
#include "tls_client.h"

namespace evt_loop {

class BusinessTester {
    public:
    BusinessTester() :
      echoserver_binary_("0.0.0.0", 50000, MessageType::BINARY),
      echoclient_binary_("localhost", 50000, MessageType::BINARY)
    {
        //TLSConnection::setSSLCertKey("./ca/certs/server.cert.pem", "./ca/private/server.key.pem", "./ca/certs/ca.cert.pem");
        TLSConnection::setSSLCertKey("server.pem", "server.pem");
        TcpCallbacksPtr echo_svr_1_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        echo_svr_1_cbs->on_msg_recvd_cb = std::bind(&BusinessTester::OnMessageRecvd_1, this, std::placeholders::_1, std::placeholders::_2);

        TcpCallbacksPtr echo_client_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        echo_client_cbs->on_msg_recvd_cb = std::bind(&BusinessTester::OnMessageRecvd_Client, this, std::placeholders::_1, std::placeholders::_2);
        echo_client_cbs->on_conn_ready_cb = std::bind(&BusinessTester::OnConnectionReady, this, std::placeholders::_1);

        echoserver_binary_.SetTcpCallbacks(echo_svr_1_cbs);
        echoclient_binary_.SetTcpCallbacks(echo_client_cbs);

        /*
        echoclient_binary_.Connect();

        echoclient_binary_.Send("hello, binary message");
        echoclient_binary_.Send("hello china");
        */
    }
    void OnSignal(SignalHandler* sh, uint32_t signo)
    {
        printf("Shutdown\n");
        EV_Singleton->StopLoop();
    }

    private:
    void OnConnectionReady(TcpConnection* conn)
    {
        conn->Send("xx");
        conn->Send("hello china 2");
    }
    void OnMessageRecvd_1(TcpConnection* conn, const Message* msg)
    {
        printf("[echoserver1] fd: %d, message: %s, length: %lu\n", conn->FD(), msg->Payload(), msg->PayloadSize());
        //conn->Send(msg->Payload(), msg->PayloadSize());
        conn->Send(*msg);
    }
    void OnMessageRecvd_Client(TcpConnection* conn, const Message* msg)
    {
        printf("[echoclient] fd: %d, message: %s, length: %lu\n", conn->FD(), msg->Payload(), msg->PayloadSize());
    }

    private:
    TLSServer echoserver_binary_;
    TLSClient echoclient_binary_;
};

}  // ns evt_loop

using namespace evt_loop;

int main(int argc, char **argv) {
  BusinessTester biz_tester;
  SignalHandler sh(SignalEvent::INT, std::bind(&BusinessTester::OnSignal, &biz_tester, std::placeholders::_1, std::placeholders::_2));

  EV_Singleton->StartLoop();

  return 0;
}


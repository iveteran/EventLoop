#include <stdio.h>
#include <iostream>

#include "el.h"
#include "console.h"

using namespace std;

namespace evt_loop {

class ConsoleDemo {
    public:
    ConsoleDemo() :
      echoserver_crlf_("0.0.0.0", 10001, MessageType::CRLF)
    {
        TcpCallbacksPtr svr_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        svr_cbs->on_conn_ready_cb = std::bind(&ConsoleDemo::OnConnectionReady, this, std::placeholders::_1);
        svr_cbs->on_msg_recvd_cb = std::bind(&ConsoleDemo::OnMessageRecvd, this, std::placeholders::_1, std::placeholders::_2);

        echoserver_crlf_.SetTcpCallbacks(svr_cbs);
        echoserver_crlf_.EnableIdleTimeout(10, std::bind(&ConsoleDemo::OnConnectionIdleTimeout, this, std::placeholders::_1, std::placeholders::_2));

        Console::Instance()->registerCommand("clients", "Show number of connected clients", std::bind(&ConsoleDemo::handleClientsCommand, this, std::placeholders::_1));
    }
    void OnSignal(SignalHandler* sh, uint32_t signo)
    {
        el_logger->info("[ConsoleDemo::OnSignal] Shutdown");
        Console::Instance()->destory(); // XXX: MUST call destory of Console manually, otherwise the terminal will be silently always
        EV_Singleton->StopLoop();
    }

    private:
    void OnConnectionReady(TcpConnection* conn)
    {
        el_logger->info("[ConsoleDemo::OnConnectionReady] fd: {}", conn->FD());
        conn->Send("hello console\n");
    }
    void OnConnectionIdleTimeout(TcpConnection* conn, uint32_t time)
    {
        el_logger->debug("[ConsoleDemo::OnConnectionIdleTimeout] fd: {}, now: {}", conn->FD(), Now());
        //conn->Disconnect();
    }
    void OnMessageRecvd(TcpConnection* conn, const Message* msg)
    {
        el_logger->debug("[ConsoleDemo::OnMessageRecvd] fd: {}, message: {}, length: {}", conn->FD(), msg->Payload(), msg->PayloadSize());
        //conn->Send(msg->Payload(), msg->PayloadSize());
        conn->Send(*msg);
    }

    int handleClientsCommand(const vector<string>& argv)
    {
        Console::Instance()->put_line("clients: ", echoserver_crlf_.GetConnectionNumber());
        return 0;
    }

    private:
    TcpServer echoserver_crlf_;
};

}  // ns evt_loop

using namespace evt_loop;

int main(int argc, char **argv) {
  ConsoleDemo console_demo;
  SignalHandler sh(SignalEvent::INT, std::bind(&ConsoleDemo::OnSignal, &console_demo, std::placeholders::_1, std::placeholders::_2));

  EV_Singleton->StartLoop();

  return 0;
}


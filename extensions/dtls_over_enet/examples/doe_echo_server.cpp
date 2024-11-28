#include "doe_server.h"
#include "doe_peer.h"
#include "eventloop/eventloop.h"
#include "eventloop/signal_handler.h"

using std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4;
using namespace evt_loop;

class DOEEchoServer {
    public:
    DOEEchoServer()
        : server_(IPVer::V4, 5000, "127.0.0.1")
    {
        server_.Init("certs/server-cert.pem", "certs/server-key.pem"/*, "certs/ca-cert.pem"*/);

        server_.SetOnPacketReceivedCallback(
                std::bind(&DOEEchoServer::OnPacketRecvd, this, _1, _2, _3));
        server_.SetOnErrorCallback(
                std::bind(&DOEEchoServer::OnError, this, _1, _2, _3));

        server_.SetOnPeerConnectedCallback(std::bind(&DOEEchoServer::OnPeerConnected, this, _1));
        server_.SetOnPeerDisconnectedCallback(std::bind(&DOEEchoServer::OnPeerDisconnected, this, _1));
        server_.SetOnPeerReadyCallback(std::bind(&DOEEchoServer::OnPeerReady, this, _1));
    }

    void OnSignal(SignalHandler* sh, uint32_t signo)
    {
        printf("Shutdown\n");
        EV_Singleton->StopLoop();
    }

    protected:
    void OnError(DOEPeer* peer, int errcode, const char* errmsg)
    {
        printf("[DOEEchoServer::OnError] error: %s\n", errmsg);
    }

    void OnPeerConnected(DOEPeer* peer)
    {
        printf("[DOEEchoServer::OnPeerConnected] peer addr: %s\n", peer->GetPeerAddr()->String().c_str());
    }

    void OnPeerDisconnected(DOEPeer* peer)
    {
        printf("[DOEEchoServer::OnPeerDisconnected] peer addr: %s\n", peer->GetPeerAddr()->String().c_str());
    }

    void OnPeerReady(DOEPeer* peer)
    {
        printf("[DOEEchoServer::OnPeerReady] peer addr: %s\n", peer->GetPeerAddr()->String().c_str());
    }

    void OnPacketRecvd(DOEPeer* peer, const char* data, size_t size)
    {
        printf("[DOEEchoServer::OnPacketRecvd] peer addr: %s\n", peer->GetPeerAddr()->String().c_str());
        printf("[DOEEchoServer::OnPacketRecvd] packet size: %ld\n", size);
        printf("[DOEEchoServer::OnPacketRecvd] received packet bytes:\n");
        printf(DumpHexWithChars(string(data, size)).c_str());
        printf("\n");

        peer->SendPacket(data, size);  // echo back
    }

    private:
    DOEServer server_;
};

int main() {
    DOEEchoServer echo_server;
    SignalHandler sh(SignalEvent::INT, std::bind(
                &DOEEchoServer::OnSignal,
                &echo_server,
                std::placeholders::_1,
                std::placeholders::_2));

    EV_Singleton->StartLoop();
    return 0;
}

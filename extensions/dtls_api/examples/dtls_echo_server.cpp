#include "dtls_server.h"
#include "eventloop/eventloop.h"
#include "eventloop/signal_handler.h"

#define ENABLE_IPV6

using std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4;
using namespace evt_loop;

class DTLSEchoServer {
    public:
    DTLSEchoServer()
        : server_(IPVer::V4, "127.0.0.1", 50000)
#ifdef ENABLE_IPV6
        , server_v6_(IPVer::V6, "::1", 50001)
#endif
    {
        server_.Init("certs/server-cert.pem", "certs/server-key.pem"/*, "certs/ca-cert.pem"*/);

        server_.SetOnPacketCallback(std::bind(&DTLSEchoServer::OnPacketRecvd, this,
                    _1, _2, _3, _4));
        server_.SetOnErrorCallback(std::bind(&DTLSEchoServer::OnError, this,
                    _1, _2, _3));

        server_.SetOnPeerConnectedCallback(std::bind(&DTLSEchoServer::OnPeerConnected, this, _1));
        server_.SetOnPeerDisconnectedCallback(std::bind(&DTLSEchoServer::OnPeerDisconnected, this, _1));
        server_.SetOnPeerReadyCallback(std::bind(&DTLSEchoServer::OnPeerReady, this, _1));

#ifdef ENABLE_IPV6
        server_v6_.Init("certs/server-cert.pem", "certs/server-key.pem"/*, "certs/ca-cert.pem"*/);

        server_v6_.SetOnPacketCallback(std::bind(&DTLSEchoServer::OnPacketRecvd, this,
                    _1, _2, _3, _4));
        server_v6_.SetOnErrorCallback(std::bind(&DTLSEchoServer::OnError, this,
                    _1, _2, _3));

        server_v6_.SetOnPeerConnectedCallback(std::bind(&DTLSEchoServer::OnPeerConnected, this, _1));
        server_v6_.SetOnPeerDisconnectedCallback(std::bind(&DTLSEchoServer::OnPeerDisconnected, this, _1));
        server_v6_.SetOnPeerReadyCallback(std::bind(&DTLSEchoServer::OnPeerReady, this, _1));
#endif
    }

    void OnSignal(SignalHandler* sh, uint32_t signo)
    {
        printf("Shutdown\n");
        EV_Singleton->StopLoop();
    }

    protected:
    void OnError(UdpPeer* peer, int errcode, const char* errmsg)
    {
        printf("[DTLSEchoServer::OnError] error: %s\n", errmsg);
    }

    void OnPeerConnected(DTLSPeer* peer)
    {
        printf("[DTLSEchoServer::OnPeerConnected] peer addr: %s\n", peer->GetPeerAddr()->String().c_str());
    }

    void OnPeerDisconnected(DTLSPeer* peer)
    {
        printf("[DTLSEchoServer::OnPeerDisconnected] peer addr: %s\n", peer->GetPeerAddr()->String().c_str());
    }

    void OnPeerReady(DTLSPeer* peer)
    {
        printf("[DTLSEchoServer::OnPeerReady] peer addr: %s\n", peer->GetPeerAddr()->String().c_str());
    }

    void OnPacketRecvd(UdpPeer* peer, const IPAddr* remote_addr, const char* data, size_t size)
    {
        printf("[DTLSEchoServer::OnPacketRecvd] fd: %d, remote addr: %s\n",
                peer->FD(), remote_addr->String().c_str());
        printf("[DTLSEchoServer::OnPacketRecvd] packet size: %ld\n", size);
        printf("[DTLSEchoServer::OnPacketRecvd] received packet bytes:\n");
        printf(DumpHexWithChars(string(data, size)).c_str());
        printf("\n");

        peer->SendPacket(remote_addr, data, size);  // echo back
    }

    private:
    DTLSServer server_;
#ifdef ENABLE_IPV6
    DTLSServer server_v6_;
#endif
};

int main() {
    DTLSEchoServer echo_server;
    SignalHandler sh(SignalEvent::INT, std::bind(
                &DTLSEchoServer::OnSignal,
                &echo_server,
                std::placeholders::_1,
                std::placeholders::_2));

    EV_Singleton->StartLoop();
    return 0;
}

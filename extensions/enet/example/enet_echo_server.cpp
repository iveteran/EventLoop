#include <stdio.h>
#include "enet_server.h"
#include "eventloop/el.h"

using namespace evt_loop;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

class ENetEchoServer {
    public:
    ENetEchoServer() : server_(IPVer::V4, 1234, "127.0.0.1")
    {
        server_.SetOnPeerConnectedCallback(std::bind(&ENetEchoServer::OnNewClient, this, _1));
        server_.SetOnPeerDisconnectedCallback(std::bind(&ENetEchoServer::OnClientDisconneted, this, _1));
        server_.SetOnPacketReceivedCallback(std::bind(&ENetEchoServer::OnReceivedPacket, this, _1, _2, _3));
    }

    void OnSignal(SignalHandler* sh, uint32_t signo)
    {
        printf("Shutdown\n");
        EV_Singleton->StopLoop();
    }

    protected:
    void OnNewClient(ENetPeer* peer) {
        char peer_ip[64];
        enet_address_get_host_ip(&peer->address, peer_ip, sizeof(peer_ip));
        printf("[ENetEchoServer::OnNewClient] %s:%d\n", peer_ip, peer->address.port);
    }
    void OnClientDisconneted(ENetPeer* peer) {
        char peer_ip[64];
        enet_address_get_host_ip(&peer->address, peer_ip, sizeof(peer_ip));
        printf("[ENetEchoServer::OnClientDisconneted] %s:%d\n", peer_ip, peer->address.port);
    }
    void OnReceivedPacket(ENetPeer* peer, const char* data, size_t size) {
        printf("[ENetEchoServer::OnPacket] size: %ld\n", size);
        printf("[ENetEchoServer::OnPacket] bytes:\n%s\n", DumpHexWithChars(string(data, size)).c_str());
        server_.SendPacket(peer, data, size);
    }

    private:
    ENetServer server_;
};

int main()
{
    ENetEchoServer echo_server;
    SignalHandler sh(SignalEvent::INT, std::bind(&ENetEchoServer::OnSignal, &echo_server, std::placeholders::_1, std::placeholders::_2));

    EV_Singleton->StartLoop();

    return 0;
}

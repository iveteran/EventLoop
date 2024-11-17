#include <stdio.h>
#include "enet_client.h"
#include "eventloop/el.h"

using namespace evt_loop;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

class ENetEchoClient {
    public:
    ENetEchoClient()
    {
        client_.SetOnClientConnectedCallback(std::bind(&ENetEchoClient::OnConnected, this, _1));
        client_.SetOnPeerDisconnectedCallback(std::bind(&ENetEchoClient::OnClientDisconneted, this, _1));
        client_.SetOnPacketReceivedCallback(std::bind(&ENetEchoClient::OnReceivedPacket, this, _1, _2, _3));
        client_.Connect(1234, "127.0.0.1");
    }

    void OnSignal(SignalHandler* sh, uint32_t signo)
    {
        printf("Disconnecting...\n");
        client_.Disconnect();
    }

    protected:
    void OnConnected(ENetPeer* peer) {
        char peer_ip[64];
        enet_address_get_host_ip(&peer->address, peer_ip, sizeof(peer_ip));
        printf("[ENetEchoClient::OnConnected] %s:%d\n", peer_ip, peer->address.port);

        const char* data = "hello world";
        client_.SendPacket(peer, data, strlen(data));
    }
    void OnClientDisconneted(ENetPeer* peer) {
        char peer_ip[64];
        enet_address_get_host_ip(&peer->address, peer_ip, sizeof(peer_ip));
        printf("[ENetEchoClient::OnClientDisconneted] %s:%d\n", peer_ip, peer->address.port);

        printf("Disconnected, shutdown\n");
        EV_Singleton->StopLoop();
    }
    void OnReceivedPacket(ENetPeer* peer, const char* data, size_t size) {
        printf("[ENetEchoClient::OnPacket] size: %ld\n", size);
        printf("[ENetEchoClient::OnPacket] bytes:\n%s\n", DumpHexWithChars(string(data, size)).c_str());
    }

    private:
    ENetClient client_;
};

int main()
{
    ENetEchoClient echo_client;
    SignalHandler sh(SignalEvent::INT, std::bind(&ENetEchoClient::OnSignal, &echo_client, std::placeholders::_1, std::placeholders::_2));

    EV_Singleton->StartLoop();

    return 0;
}

#include "dtls_client.h"
#include "eventloop/eventloop.h"
#include "eventloop/signal_handler.h"

#define ENABLE_IPV6

using std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4;
using namespace evt_loop;

class DTLSEchoClient {
    public:
    DTLSEchoClient()
        : client_(IPVer::V4, "127.0.0.1", 50000)
#ifdef ENABLE_IPV6
        , client_v6_(IPVer::V6, "::1", 50001)
#endif
    {
        //client_.Init("certs/client-cert.pem", "certs/client-key.pem", "certs/ca-cert.pem");
        client_.Init(nullptr, nullptr, "certs/ca-cert.pem");

        client_.SetOnPacketCallback(std::bind(&DTLSEchoClient::OnPacketRecvd, this,
                    _1, _2, _3, _4));
        client_.SetOnErrorCallback(std::bind(&DTLSEchoClient::OnError, this,
                    _1, _2, _3));

        client_.SetOnPeerDisconnectedCallback(std::bind(&DTLSEchoClient::OnPeerDisconnected, this, _1));
        client_.SetOnPeerReadyCallback(std::bind(&DTLSEchoClient::OnPeerReady, this, _1));

        bool success = client_.Connect();
        if (success) {
            const char* data = "hello from dtls client";
            client_.SendPacket(data, strlen(data));
        }
#ifdef ENABLE_IPV6
        //client_.Init("certs/client-cert.pem", "certs/client-key.pem", "certs/ca-cert.pem");
        client_v6_.Init(nullptr, nullptr, "certs/ca-cert.pem");

        client_v6_.SetOnPacketCallback(std::bind(&DTLSEchoClient::OnPacketRecvd, this,
                    _1, _2, _3, _4));
        client_v6_.SetOnErrorCallback(std::bind(&DTLSEchoClient::OnError, this,
                    _1, _2, _3));

        client_v6_.SetOnPeerDisconnectedCallback(std::bind(&DTLSEchoClient::OnPeerDisconnected, this, _1));
        client_v6_.SetOnPeerReadyCallback(std::bind(&DTLSEchoClient::OnPeerReady, this, _1));

        success = client_v6_.Connect();
        if (success) {
            const char* data = "hello from dtls client ipv6";
            client_v6_.SendPacket(data, strlen(data));
        }
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
        printf("[DTLSEchoClient::OnError] error: %s\n", errmsg);
    }

    void OnPeerDisconnected(DTLSPeer* peer)
    {
        printf("[DTLSEchoClient::OnPeerDisconnected] peer addr: %s\n", peer->GetPeerAddr()->String().c_str());
    }

    void OnPeerReady(DTLSPeer* peer)
    {
        printf("[DTLSEchoClient::OnPeerReady] peer addr: %s\n", peer->GetPeerAddr()->String().c_str());
    }

    void OnPacketRecvd(UdpPeer* peer, const IPAddr* remote_addr, const char* data, size_t size)
    {
        printf("[DTLSEchoClient::OnPacketRecvd] fd: %d, remote addr: %s\n",
                peer->FD(), remote_addr->String().c_str());
        printf("[DTLSEchoClient::OnPacketRecvd] packet size: %ld\n", size);
        printf("[DTLSEchoClient::OnPacketRecvd] received packet bytes:\n");
        printf(DumpHexWithChars(string(data, size)).c_str());
        printf("\n");

        //peer->SendPacket(remote_addr, data, size);  // echo back
    }

    private:
    DTLSClient client_;
#ifdef ENABLE_IPV6
    DTLSClient client_v6_;
#endif
};

int main() {
    DTLSEchoClient echo_client;
    SignalHandler sh(SignalEvent::INT, std::bind(
                &DTLSEchoClient::OnSignal,
                &echo_client,
                std::placeholders::_1,
                std::placeholders::_2));

    EV_Singleton->StartLoop();
    return 0;
}

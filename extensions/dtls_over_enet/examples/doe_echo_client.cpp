#include "doe_client.h"
#include "doe_peer.h"
#include "eventloop/el.h"

using std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4;
using namespace evt_loop;

void OneshotTimerHandler(TimerEvent* timer)
{
    printf("[OneshotTimerHandler] triggered, shutdown\n");
    EV_Singleton->StopLoop();
}

class DOEEchoClient {
    public:
    DOEEchoClient()
        //: client_(IPVer::V4, "127.0.0.1", 5000)
    {
        //client_.Init("certs/client-cert.pem", "certs/client-key.pem", "certs/ca-cert.pem");
        client_.Init(nullptr, nullptr, "certs/ca-cert.pem");

        client_.SetOnPacketReceivedCallback(
                std::bind(&DOEEchoClient::OnPacketRecvd, this, _1, _2, _3));
        client_.SetOnErrorCallback(
                std::bind(&DOEEchoClient::OnError, this, _1, _2, _3));

        client_.SetOnPeerConnectedCallback(std::bind(&DOEEchoClient::OnPeerConnected, this, _1));
        client_.SetOnPeerDisconnectedCallback(std::bind(&DOEEchoClient::OnPeerDisconnected, this, _1));
        client_.SetOnPeerReadyCallback(std::bind(&DOEEchoClient::OnPeerReady, this, _1));

        bool success = client_.Connect(IPVer::V4, "127.0.0.1", 5000);
        if (success) {
            //const char* data = "hello from DTLS over ENet client";
            //client_.SendPacket(data, strlen(data));
        }
    }

    void OnSignal(SignalHandler* sh, uint32_t signo)
    {
        printf("Shutdown\n");
        client_.Disconnect();

        oneshot_timer_ = new OneshotTimer(TimeVal(3, 0), &OneshotTimerHandler);
        oneshot_timer_->Start();
    }

    protected:
    void OnError(DOEPeer* peer, int errcode, const char* errmsg)
    {
        printf("[DOEEchoClient::OnError] error: %s\n", errmsg);
    }

    void OnPeerConnected(DOEPeer* peer)
    {
        printf("[DOEEchoClient::OnPeerConnected] peer addr: %s\n", peer->GetPeerAddr()->String().c_str());
    }

    void OnPeerDisconnected(DOEPeer* peer)
    {
        printf("[DOEEchoClient::OnPeerDisconnected] peer addr: %s\n", peer->GetPeerAddr()->String().c_str());
    }

    void OnPeerReady(DOEPeer* peer)
    {
        printf("[DOEEchoClient::OnPeerReady] peer addr: %s\n", peer->GetPeerAddr()->String().c_str());

        const char* data = "hello from DTLS over ENet client";
        peer->SendPacket(data, strlen(data));
    }

    void OnPacketRecvd(DOEPeer* peer, const char* data, size_t size)
    {
        printf("[DOEEchoClient::OnPacketRecvd] peer addr: %s\n", peer->GetPeerAddr()->String().c_str());
        printf("[DOEEchoClient::OnPacketRecvd] packet size: %ld\n", size);
        printf("[DOEEchoClient::OnPacketRecvd] received packet bytes:\n");
        printf(DumpHexWithChars(string(data, size)).c_str());
        printf("\n");

        //peer->SendPacket(remote_addr, data, size);  // echo back
    }

    private:
    DOEClient client_;
    OneshotTimer* oneshot_timer_;
};

int main() {
    DOEEchoClient echo_client;
    SignalHandler sh(SignalEvent::INT, std::bind(
                &DOEEchoClient::OnSignal,
                &echo_client,
                std::placeholders::_1,
                std::placeholders::_2));

    EV_Singleton->StartLoop();
    return 0;
}

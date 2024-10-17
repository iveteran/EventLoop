#include <stdio.h>

#include "kcp_peer.h"
#include "eventloop/udp_peer.h"
#include "eventloop/el.h"
#include "eventloop/logger.h"

using namespace evt_loop;

class KcpEchoClient {
    public:
    KcpEchoClient(uint32_t sess_id = 123456)
        : client_(sess_id, 98)
    {
        client_.InitUdpPeer("localhost", 10003, UdpPeer::Mode::REMOTE);
        client_.SetOnPacketCallback(std::bind(&KcpEchoClient::OnPacketRecvd_Client, this,
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        //client_.SetOnErrorCallback(std::bind(&KcpEchoClient::OnError, this,
        //            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        const char* data = "Hello World";
        //const char* data = "Hello World, abcdefdhijklmn";
        size_t size = strlen(data);
        client_.SendPacket(data, size);

        el_logger->debug("[KcpEchoClient::KcpEchoClient] packet size: {}", size);
        el_logger->debug("[KcpEchoClient::KcpEchoClient] sent packet bytes:");
        el_logger->output(DumpHexWithChars(string(data, size))).eol();
    }
    void OnSignal(SignalHandler* sh, uint32_t signo)
    {
        el_logger->info("[KcpEchoClient::OnSignal] Shutdown");
        EV_Singleton->StopLoop();
    }

    private:
    void OnError(KcpPeer* kcp_peer, int errcode, const char* errmsg)
    {
        el_logger->debug("[KcpEchoClient::OnError] error: {}", errmsg);
    }

    void OnPacketRecvd_Client(KcpPeer* kcp_peer, const char* data, size_t size)
    {
        el_logger->debug("[KcpEchoClient::OnPacketRecvd_Client] session id: {}", kcp_peer->GetSessionId());
        el_logger->debug("[KcpEchoClient::OnPacketRecvd_Client] packet size: {}", size);
        el_logger->debug("[KcpEchoClient::OnPacketRecvd_Client] received packet bytes:");
        el_logger->output(DumpHexWithChars(data, size)).eol();

        //kcp_peer->SendPacket(data, size);  // echo message
    }

    private:
    KcpPeer client_;
};

int main(int argc, char **argv) {
  KcpEchoClient server;
  SignalHandler sh(SignalEvent::INT, std::bind(&KcpEchoClient::OnSignal, &server, std::placeholders::_1, std::placeholders::_2));

  EV_Singleton->StartLoop();

  return 0;
}


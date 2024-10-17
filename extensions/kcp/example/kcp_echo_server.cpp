#include <stdio.h>

#include "kcp_peer.h"
#include "eventloop/udp_peer.h"
#include "eventloop/el.h"
#include "eventloop/logger.h"

#define INTEGRATE_CLIENT 0

using namespace evt_loop;

class KcpEchoServer {
    public:
    KcpEchoServer(uint32_t sess_id = 123456)
        : server_(sess_id, 99)
#if INTEGRATE_CLIENT
        , client_(sess_id, 98)
#endif
    {
        server_.InitUdpPeer("localhost", 10003, UdpPeer::Mode::LOCAL);
        server_.SetOnPacketCallback(std::bind(&KcpEchoServer::OnPacketRecvd_Server, this,
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        //server_.SetOnErrorCallback(std::bind(&KcpEchoServer::OnError, this,
        //            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

#if INTEGRATE_CLIENT
        client_.InitUdpPeer("localhost", 10003, UdpPeer::Mode::REMOTE);
        client_.SetOnPacketCallback(std::bind(&KcpEchoServer::OnPacketRecvd_Client, this,
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        //client_.SetOnErrorCallback(std::bind(&KcpEchoServer::OnError, this,
        //            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        const char* data = "Hello World";
        //const char* data = "Hello World, abcdefdhijklmn";
        size_t size = strlen(data);
        client_.SendPacket(data, size);

        el_logger->debug("[KcpEchoServer::KcpEchoServer] client: packet size: {}", size);
        el_logger->debug("[KcpEchoServer::KcpEchoServer] client: sent packet bytes:");
        el_logger->output(DumpHexWithChars(string(data, size))).eol();
#endif
    }
    void OnSignal(SignalHandler* sh, uint32_t signo)
    {
        el_logger->info("[KcpEchoServer::OnSignal] Shutdown");
        EV_Singleton->StopLoop();
    }

    private:
    void OnError(KcpPeer* kcp_peer, int errcode, const char* errmsg)
    {
        el_logger->debug("[KcpEchoServer::OnError] error: {}", errmsg);
    }
    void OnPacketRecvd_Server(KcpPeer* kcp_peer, const char* data, size_t size)
    {
        el_logger->debug("[KcpEchoServer::OnPacketRecvd_Server] session id: {}", kcp_peer->GetSessionId());
        el_logger->debug("[KcpEchoServer::OnPacketRecvd_Server] packet size: {}", size);
        el_logger->debug("[KcpEchoServer::OnPacketRecvd_Server] received packet bytes:");
        el_logger->output(DumpHexWithChars(string(data, size))).eol();

        kcp_peer->SendPacket(data, size);  // echo message

        el_logger->debug("[KcpEchoServer::OnPacketRecvd_Server] server: packet size: {}", size);
        el_logger->debug("[KcpEchoServer::OnPacketRecvd_Server] server: sent packet bytes:");
        el_logger->output(DumpHexWithChars(string(data, size))).eol();
    }

#if INTEGRATE_CLIENT
    void OnPacketRecvd_Client(KcpPeer* kcp_peer, const char* data, size_t size)
    {
        el_logger->debug("[KcpEchoServer::OnPacketRecvd_Client] session id: {}", kcp_peer->GetSessionId());
        el_logger->debug("[KcpEchoServer::OnPacketRecvd_Client] packet size: {}", size);
        el_logger->debug("[KcpEchoServer::OnPacketRecvd_Client] received packet bytes:");
        el_logger->output(DumpHexWithChars(data, size)).eol();

        //kcp_peer->SendPacket(data, size);  // echo message
    }
#endif

    private:
    KcpPeer server_;
#if INTEGRATE_CLIENT
    KcpPeer client_;
#endif
};

int main(int argc, char **argv) {
  KcpEchoServer server;
  SignalHandler sh(SignalEvent::INT, std::bind(&KcpEchoServer::OnSignal, &server, std::placeholders::_1, std::placeholders::_2));

  EV_Singleton->StartLoop();

  return 0;
}


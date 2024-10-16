#include <stdio.h>

#include "eventloop/udp_peer.h"
#include "eventloop/el.h"
#include "eventloop/logger.h"

using namespace evt_loop;

class UdpEchoServer {
    public:
    UdpEchoServer()
    {
        server_.Init("0.0.0.0", 10001, UdpPeer::Mode::LOCAL);
        server_.SetOnPacketCallback(std::bind(&UdpEchoServer::OnPacketRecvd_Server, this,
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        server_.SetOnErrorCallback(std::bind(&UdpEchoServer::OnError, this,
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        client_.Init("0.0.0.0", 10001, UdpPeer::Mode::REMOTE);
        client_.SetOnPacketCallback(std::bind(&UdpEchoServer::OnPacketRecvd_Client, this,
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        client_.SetOnErrorCallback(std::bind(&UdpEchoServer::OnError, this,
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        const char* data = "Hello World";
        //const char* data = "Hello World, abcdefdhijklmn";
        size_t size = strlen(data);
        client_.SendPacket(data, size);

        el_logger->debug("[UdpEchoServer::UdpEchoServer] client: packet size: {}", size);
        el_logger->debug("[UdpEchoServer::UdpEchoServer] client: sent packet bytes:");
        el_logger->output(DumpHexWithChars(string(data, size))).eol();
    }
    void OnSignal(SignalHandler* sh, uint32_t signo)
    {
        el_logger->info("[UdpEchoServer::OnSignal] Shutdown");
        EV_Singleton->StopLoop();
    }

    private:
    void OnError(UdpPeer* udp_peer, int errcode, const char* errmsg)
    {
        el_logger->debug("[UdpEchoServer::OnError] error: {}", errmsg);
    }
    void OnPacketRecvd_Server(UdpPeer* udp_peer, const PeerAddr* remote_addr, const char* data, size_t size)
    {
        el_logger->debug("[UdpEchoServer::OnPacketRecvd_Server] fd: {}, remote addr: {}",
                udp_peer->FD(), remote_addr->String());
        el_logger->debug("[UdpEchoServer::OnPacketRecvd_Server] packet size: {}", size);
        el_logger->debug("[UdpEchoServer::OnPacketRecvd_Server] received packet bytes:");
        el_logger->output(DumpHexWithChars(string(data, size))).eol();

        udp_peer->SendPacket(remote_addr, data, size);  // echo message
    }

    void OnPacketRecvd_Client(UdpPeer* udp_peer, const PeerAddr* remote_addr, const char* data, size_t size)
    {
        el_logger->debug("[UdpEchoServer::OnPacketRecvd_Client] fd: {}, remote addr: {}",
                udp_peer->FD(), remote_addr->String());
        el_logger->debug("[UdpEchoServer::OnPacketRecvd_Client] packet size: {}", size);
        el_logger->debug("[UdpEchoServer::OnPacketRecvd_Client] received packet bytes:");
        el_logger->output(DumpHexWithChars(data, size)).eol();

        //udp_peer->SendPacket(remote_addr, data, size);  // echo message
    }

    private:
    UdpPeer4 server_;
    UdpPeer4 client_;
};

int main(int argc, char **argv) {
  UdpEchoServer server;
  SignalHandler sh(SignalEvent::INT, std::bind(&UdpEchoServer::OnSignal, &server, std::placeholders::_1, std::placeholders::_2));

  EV_Singleton->StartLoop();

  return 0;
}


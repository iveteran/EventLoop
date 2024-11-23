#include "dtls_server.h"
#include "dtls_peer.h"
#include "ssl_setup.h"
#include "eventloop/logger.h"

namespace evt_loop {

static SSL_CTX* ssl_ctx_ = nullptr;

DTLSServer::DTLSServer(IPVer ip_ver, const char* host, uint16_t port)
    : UdpPeer(ip_ver, host, port, UdpPeer::Mode::SERVER)
{
}

DTLSServer::~DTLSServer()
{
    for (auto [_, peer] : peers_) {
        delete peer;
    }
    peers_.clear();
    //EVP_cleanup();
}

void DTLSServer::Init(const char* cert_file, const char* key_file, const char* ca_file)
{
    if (! ssl_ctx_) {
        const bool is_server = true;
        ssl_ctx_ = SetupSSLContext(is_server, cert_file, key_file, ca_file);
    }
}

void DTLSServer::OnPeerConnected(DTLSPeer* peer)
{
    if (on_peer_connected_cb_) {
        on_peer_connected_cb_(peer);
    }
}

void DTLSServer::OnPeerDisconnected(DTLSPeer* peer)
{
    auto peer_key = peer->GetPeerAddr()->String();
    peers_.erase(peer_key);
    el_logger->info("[DTLSServer::OnPeerDisconnected] peer disconnected: {}", peer_key);

    if (on_peer_disconnected_cb_) {
        on_peer_disconnected_cb_(peer);
    }
}

size_t DTLSServer::_receive_packet(const char* buf, size_t bufsize, int flags,
        IPAddr* peer_addr)
{
    bool success = ObtainPeerAddress((struct sockaddr *)peer_addr->SockAddr());
    if (! success) {
        return 0;
    }
    auto peer_key = peer_addr->String();
    el_logger->debug("[DTLSServer::_receive_packet] client key: {}", peer_key);

    bool is_server = true;
    DTLSPeer* peer;
    if (auto iter = peers_.find(peer_key); iter == peers_.end()) {
        el_logger->info("[DTLSServer::_receive_packet] peer connected: {}", peer_key);
        peer = new DTLSPeer(ssl_ctx_, is_server, FD(), peer_addr);
        peer->SetOnReadyCallback(on_peer_ready_cb_);
        peer->SetOnDisconnectedCallback(std::bind(&DTLSServer::OnPeerDisconnected, this, std::placeholders::_1));
        peers_[peer_key] = peer;
        OnPeerConnected(peer);
    } else {
        peer = iter->second;
    }
    return peer->ReceivePacket(buf, bufsize);
}

size_t DTLSServer::_send_packet(const char* data, size_t size, int flags,
        const IPAddr* peer_addr)
{
    auto peer_key = peer_addr->String();
    el_logger->debug("[DTLSServer::_send_packet] client key: {}", peer_key);
    if (auto iter = peers_.find(peer_key); iter != peers_.end()) {
        auto peer = iter->second;
        return peer->SendPacket(data, size);
    } else {
        el_logger->error("[DTLSServer::_send_packet] unknown client, client key: {}", peer_key);
        return 0;
    }
}

bool DTLSServer::ObtainPeerAddress(struct sockaddr* peer_addr)
{
    socklen_t client_len = sizeof(*peer_addr);
    char buffer[BUFFER_SIZE];

    // Receive initial datagram
    int len = recvfrom(FD(), buffer, BUFFER_SIZE, MSG_PEEK, peer_addr, &client_len);
    if (len < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("recvfrom failed");
        }
        return false;
    }

    return true;
}

}  // evt_loop

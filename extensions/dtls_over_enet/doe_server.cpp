#include "doe_server.h"
#include "doe_peer.h"
#include "../dtls_api/ssl_setup.h"
#include "eventloop/logger.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace evt_loop {

static SSL_CTX* ssl_ctx_ = nullptr;

static string generate_peer_key(const char* ip, uint16_t port)
{
    char peer_key[64];
    snprintf(peer_key, sizeof(peer_key), "%s:%d", ip, port);
    return string(peer_key);
}

DOEServer::DOEServer(IPVer ip_ver, uint16_t port, const char* host)
    : enet_server_(ip_ver, port, host)
{
    enet_server_.SetOnPeerConnectedCallback(std::bind(&DOEServer::OnEnetNewClient, this, _1));
    enet_server_.SetOnPeerDisconnectedCallback(std::bind(&DOEServer::OnENetPeerDisconnected, this, _1));
    enet_server_.SetOnPacketReceivedCallback(std::bind(&DOEServer::OnENetReceivedPacket, this, _1, _2, _3));
}

DOEServer::~DOEServer()
{
    Destroy();
}

void DOEServer::Init(const char* cert_file, const char* key_file, const char* ca_file)
{
    if (! ssl_ctx_) {
        const bool is_server = true;
        ssl_ctx_ = SetupSSLContext(is_server, cert_file, key_file, ca_file);
    }
}

void DOEServer::Destroy()
{
    for (auto [_, peer] : peers_) {
        delete peer;
    }
    peers_.clear();
}

void DOEServer::OnEnetNewClient(ENetPeer* enet_peer)
{
    char peer_ip[64];
    enet_address_get_host_ip(&enet_peer->address, peer_ip, sizeof(peer_ip));
    el_logger->debug("[DOEServer::OnEnetNewClient] enet peer: {}:{}", peer_ip, enet_peer->address.port);

    auto peer_key = generate_peer_key(peer_ip, enet_peer->address.port);
    const bool is_server = true;

    if (auto iter = peers_.find(peer_key); iter == peers_.end()) {
        auto peer = new DOEPeer(enet_peer, ssl_ctx_, is_server);
        auto dtls_peer = peer->GetDTLSPeer();
        dtls_peer->SetOnReadyCallback(std::bind(&DOEServer::OnDTLSPeerReady, this, std::placeholders::_1));
        dtls_peer->SetOnDisconnectedCallback(std::bind(&DOEServer::OnDTLSPeerDisconnected, this, std::placeholders::_1));
        dtls_peer->Handshake();
        peer->FlushPeerOutgoing();
        el_logger->debug("[DOEServer::OnEnetNewClient] doe peer: {}", peer_key);

        //peer->SetOnReadyCallback(on_peer_ready_cb_);
        //peer->SetOnDisconnectedCallback(std::bind(&POEServer::OnPeerDisconnected, this, std::placeholders::_1));
        peers_[peer_key] = peer;
        OnPeerConnected(peer);
    }
}

void DOEServer::OnENetPeerDisconnected(ENetPeer* enet_peer)
{
    char peer_ip[64];
    enet_address_get_host_ip(&enet_peer->address, peer_ip, sizeof(peer_ip));
    auto peer_key = generate_peer_key(peer_ip, enet_peer->address.port);

    el_logger->info("[DTLSServer::OnENetPeerDisconnected] peer disconnected: {}", peer_key);

    if (auto iter = peers_.find(peer_key); iter != peers_.end()) {
        auto peer = iter->second;
        if (on_peer_disconnected_cb_) {
            on_peer_disconnected_cb_(peer);
        }
        //peer->GetDTLSPeer()->Disconnect();
        peers_.erase(iter);
    } else {
        el_logger->error("[DOEServer::OnENetPeerDisconnected] Unknown peer: {}", peer_key);
    }
}

void DOEServer::OnENetReceivedPacket(ENetPeer* enet_peer, const char* data, size_t size)
{
    el_logger->debug("[DOEServer::OnENetReceivedPacket] size: {}", size);
    //printf("[DOEServer::OnPacket] bytes:\n%s\n", DumpHexWithChars(string(data, size)).c_str());

    char peer_ip[64];
    enet_address_get_host_ip(&enet_peer->address, peer_ip, sizeof(peer_ip));
    auto peer_key = generate_peer_key(peer_ip, enet_peer->address.port);

    if (auto iter = peers_.find(peer_key); iter != peers_.end()) {
        auto peer = iter->second;
        char obuf[4096];
        size_t output_size = peer->DecryptPacket(data, size, obuf, sizeof(obuf));
        if (output_size > 0 && on_packet_received_cb_) {
            on_packet_received_cb_(peer, obuf, output_size);
        }
    } else {
        el_logger->error("[DOEServer::OnENetReceivedPacket] Unknown peer: {}", peer_key);
    }
}

void DOEServer::OnPeerConnected(DOEPeer* peer)
{
    if (on_peer_connected_cb_) {
        on_peer_connected_cb_(peer);
    }
}

void DOEServer::OnDTLSPeerReady(DTLSPeer* dtls_peer)
{
    auto peer_key = dtls_peer->GetPeerAddr()->String();
    if (auto iter = peers_.find(peer_key); iter != peers_.end()) {
        auto peer = iter->second;
        if (on_peer_ready_cb_) {
            on_peer_ready_cb_(peer);
        }
    } else {
        el_logger->error("[DOEServer::OnDTLSPeerReady] Unknown peer: {}", peer_key);
    }
}

void DOEServer::OnDTLSPeerDisconnected(DTLSPeer* dtls_peer)
{
    auto peer_key = dtls_peer->GetPeerAddr()->String();
    if (auto iter = peers_.find(peer_key); iter != peers_.end()) {
        auto peer = iter->second;
        if (on_peer_disconnected_cb_) {
            on_peer_disconnected_cb_(peer);
        }
        //peer->GetENetPeer()->Disconnect();
        peers_.erase(iter);
    } else {
        el_logger->error("[DOEServer::OnDTLSPeerDisconnected] Unknown peer: {}", peer_key);
    }
}

} // NS evt_loop

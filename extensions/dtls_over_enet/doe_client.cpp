#include "doe_client.h"
#include "doe_peer.h"
#include "../dtls_api/ssl_setup.h"
#include "eventloop/logger.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace evt_loop {

static SSL_CTX* ssl_ctx_ = nullptr;

DOEClient::DOEClient()
{
    enet_client_.SetOnPeerConnectedCallback(std::bind(&DOEClient::OnENetPeerConnected, this, _1));
    enet_client_.SetOnPeerDisconnectedCallback(std::bind(&DOEClient::OnENetPeerDisconnected, this, _1));
    enet_client_.SetOnPacketReceivedCallback(std::bind(&DOEClient::OnENetPeerReceivedPacket, this, _1, _2, _3));
}

DOEClient::~DOEClient()
{
    Destroy();
}

void DOEClient::Init(const char* cert_file, const char* key_file, const char* ca_file)
{
    if (! ssl_ctx_) {
        const bool is_server = false;  // is client
        ssl_ctx_ = SetupSSLContext(is_server, cert_file, key_file, ca_file);
    }
}

void DOEClient::Destroy()
{
    delete peer_;
    peer_ = nullptr;
}

bool DOEClient::Connect(IPVer ip_ver, const char* host, uint16_t port)
{
    return enet_client_.Connect(ip_ver, port, host);
}

void DOEClient::Disconnect()
{
    return enet_client_.Disconnect();
}

void DOEClient::OnENetPeerConnected(ENetPeer* enet_peer)
{
    char peer_ip[64];
    enet_address_get_host_ip(&enet_peer->address, peer_ip, sizeof(peer_ip));
    el_logger->debug("[DOEClient::OnENetPeerConnected] {}:{}", peer_ip, enet_peer->address.port);

    const bool is_server = false;  // is client
    peer_ = new DOEPeer(enet_peer, ssl_ctx_, is_server);
    auto dtls_peer = peer_->GetDTLSPeer();
    dtls_peer->SetOnReadyCallback(std::bind(&DOEClient::OnDTLSPeerReady, this, std::placeholders::_1));
    dtls_peer->SetOnDisconnectedCallback(std::bind(&DOEClient::OnDTLSPeerDisconnected, this, std::placeholders::_1));
    dtls_peer->Handshake();
    peer_->FlushPeerOutgoing();

    OnPeerConnected(peer_);
}

void DOEClient::OnENetPeerDisconnected(ENetPeer* enet_peer)
{
    char peer_ip[64];
    enet_address_get_host_ip(&enet_peer->address, peer_ip, sizeof(peer_ip));
    el_logger->debug("[DOEClient::OnENetPeerDisconnected] {}:{}", peer_ip, enet_peer->address.port);

    if (on_peer_disconnected_cb_) {
        on_peer_disconnected_cb_(peer_);
    }
    //peer->GetDTLSPeer()->Disconnect();
}

void DOEClient::OnENetPeerReceivedPacket(ENetPeer* enet_peer, const char* data, size_t size)
{
    el_logger->debug("[DOEClient::OnENetPeerReceivedPacket] size: {}", size);
    //printf("[DOEClient::OnENetPeerReceivedPacket] bytes:\n%s\n", DumpHexWithChars(string(data, size)).c_str());

    char obuf[4096];
    size_t output_size = peer_->DecryptPacket(data, size, obuf, sizeof(obuf));
    if (output_size > 0 && on_packet_received_cb_) {
        on_packet_received_cb_(peer_, obuf, output_size);
    }
}

void DOEClient::OnPeerConnected(DOEPeer* peer)
{
    if (on_peer_connected_cb_) {
        on_peer_connected_cb_(peer);
    }
}

void DOEClient::OnDTLSPeerReady(DTLSPeer* dtls_peer)
{
    el_logger->debug("[DOEClient::OnDTLSPeerReady] {}", dtls_peer->GetPeerAddr()->String());

    if (on_peer_ready_cb_) {
        on_peer_ready_cb_(peer_);
    }
}

void DOEClient::OnDTLSPeerDisconnected(DTLSPeer* dtls_peer)
{
    el_logger->debug("[DOEClient::OnDTLSPeerDisconnected] {}", dtls_peer->GetPeerAddr()->String());

    //peer->GetENetPeer()->Disconnect();

    if (on_peer_disconnected_cb_) {
        on_peer_disconnected_cb_(peer_);
    }
}

}  // NS evt_loop

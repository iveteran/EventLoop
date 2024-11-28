#include "../dtls_api/dtls_peer.h"
#include "doe_peer.h"
#include "eventloop/logger.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace evt_loop {

// Modified BIO callbacks
static int bio_write(BIO* bio, const char* data, int len) {
    printf("--> bio_write, len: %d\n", len);
    auto doe_peer = static_cast<DOEPeer*>(BIO_get_data(bio));
    if (!doe_peer) return -1;

    auto enet_peer = doe_peer->GetENetPeer();
    if (!enet_peer) return -1;

    // Create and send ENet packet
    ENetPacket* packet = enet_packet_create(data, len, ENET_PACKET_FLAG_RELIABLE);
    if (enet_peer_send(enet_peer, 0, packet) < 0) {
        enet_packet_destroy(packet);
        return -1;
    }
    enet_host_flush(enet_peer->host);
    return len;
}

static int bio_read(BIO* bio, char* data, int len) {
    printf("--> bio_read, buf len: %d\n", len);
    auto doe_peer = static_cast<DOEPeer*>(BIO_get_data(bio));
    if (!doe_peer) return -1;

    auto& incoming_buffer = doe_peer->IncomingBuffer();
    printf("--> bio_read, incoming_buffer size: %ld\n", incoming_buffer.size());
    if (incoming_buffer.empty()) {
        BIO_set_retry_read(bio);
        return -1;
    }

    int copy_len = std::min(len, static_cast<int>(incoming_buffer.size()));
    printf("--> bio_read, read len: %d\n", copy_len);
    std::copy(incoming_buffer.begin(), incoming_buffer.begin() + copy_len, data);
    incoming_buffer.erase(incoming_buffer.begin(), incoming_buffer.begin() + copy_len);
    return copy_len;
}

static long bio_ctrl(BIO* bio, int cmd, long num, void* ptr) {
    //el_logger->debug("bio_ctrl called with cmd: {}", cmd);

    auto* doe_peer = static_cast<DOEPeer*>(BIO_get_data(bio));
    if (!doe_peer) {
        el_logger->error("bio_ctrl: No doe_peer data");
        return 0;
    }
    auto enet_peer = doe_peer->GetENetPeer();
    if (!enet_peer) return 0;

    switch (cmd) {
        case BIO_CTRL_DGRAM_SET_NEXT_TIMEOUT:
        case BIO_CTRL_DGRAM_SET_RECV_TIMEOUT:
            return 1;
        case BIO_CTRL_FLUSH:
            enet_host_flush(enet_peer->host);
            return 1;
        case BIO_CTRL_DGRAM_QUERY_MTU:
            return 1200; // Return your preferred MTU
        case BIO_CTRL_DGRAM_GET_MTU:
            return 1200;
        default:
            //el_logger->warn("bio_ctrl: Unhandled command: {}", cmd);
            return 0;
    }
}

static int bio_create(BIO* bio) {
    BIO_set_init(bio, 1);
    return 1;
}

static int bio_destroy(BIO* bio) {
    return 1;
}

// Custom BIO methods
static BIO_METHOD* create_bio_methods() {
    BIO_METHOD* methods = BIO_meth_new(BIO_TYPE_DGRAM, "DTLS over ENet");
    BIO_meth_set_write(methods, bio_write);
    BIO_meth_set_read(methods, bio_read);
    BIO_meth_set_ctrl(methods, bio_ctrl);
    BIO_meth_set_create(methods, bio_create);
    BIO_meth_set_destroy(methods, bio_destroy);
    return methods;
}

DOEPeer::DOEPeer(ENetPeer* enet_peer, SSL_CTX* ssl_ctx, bool is_server)
    : enet_peer_(enet_peer), is_server_(is_server)
{
    char peer_ip[64];
    enet_address_get_host_ip(&(enet_peer->address), peer_ip, sizeof(peer_ip));
    peer_addr_ = new IPAddr4(peer_ip, enet_peer->address.port);

    auto bio_methods = create_bio_methods();
    const bool enable_debug = true;
    dtls_peer_ = new DTLSPeer(ssl_ctx, is_server, peer_addr_,
            bio_methods, (void*)this, enable_debug);
}

DOEPeer::~DOEPeer() {
    delete dtls_peer_;
    delete peer_addr_;
}

size_t DOEPeer::SendPacket(const char* data, size_t size)
{
    size_t tx_size = dtls_peer_->SendPacket(data, size);
    if (tx_size > 0) {
        FlushPeerOutgoing();
    }
    return tx_size;
}

size_t DOEPeer::DecryptPacket(const char* data, size_t size, char* output, size_t obuf_size)
{
    // Add received data to peer's incoming buffer
    incoming_buffer_.insert(incoming_buffer_.end(), data, data + size);

    /*
    if (! handshake_done) {
        dtls_peer_->Handshake();
    } else {
        dtls_peer_->ReceivePacket(data, size);
    }
    */
    return dtls_peer_->ReceivePacket(output, obuf_size);
}

bool DOEPeer::FlushPeerOutgoing()
{
    printf("FlushPeerOutgoing, buffer size: %ld\n", outgoing_buffer_.size());
    if (outgoing_buffer_.empty()) {
        return true;
    }

    // Create and send ENet packet with encrypted data
    ENetPacket* packet = enet_packet_create(
            outgoing_buffer_.data(),
            outgoing_buffer_.size(),
            ENET_PACKET_FLAG_RELIABLE
            );

    if (enet_peer_send(enet_peer_, 0, packet) < 0) {
        enet_packet_destroy(packet);
        return false;
    }

    outgoing_buffer_.clear();
    return true;
}

}  // NS evt_loop

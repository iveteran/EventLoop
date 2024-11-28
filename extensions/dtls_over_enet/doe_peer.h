#ifndef _DOE_PEER_H
#define _DOE_PEER_H

#include <enet/enet.h>

// forward declaration for SSL struct
struct ssl_ctx_st;
typedef struct ssl_ctx_st SSL_CTX;

namespace evt_loop {

class DTLSPeer;

class DOEPeer
{
    public:
    DOEPeer(ENetPeer* enet_peer, SSL_CTX* ssl_ctx, bool is_server);
    ~DOEPeer();

    DTLSPeer* GetDTLSPeer() { return dtls_peer_; }
    ENetPeer* GetENetPeer() { return enet_peer_; }

    const IPAddr* GetPeerAddr() const { return peer_addr_; }

    size_t SendPacket(const char* data, size_t size);

    size_t DecryptPacket(const char* data, size_t size, char* output, size_t obuf_size);
    std::vector<uint8_t>& IncomingBuffer() { return incoming_buffer_; }
    std::vector<uint8_t>& OutgoingBuffer() { return outgoing_buffer_; }

    bool FlushPeerOutgoing();

    private:
    ENetPeer* enet_peer_ = nullptr;
    DTLSPeer* dtls_peer_ = nullptr;
    bool is_server_;
    IPAddr* peer_addr_ = nullptr;

    // Buffer for DTLS operations
    std::vector<uint8_t> incoming_buffer_;
    std::vector<uint8_t> outgoing_buffer_;
};

} // NS evt_loop

#endif // _DOE_PEER_H

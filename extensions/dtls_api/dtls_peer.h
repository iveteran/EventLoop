#ifndef _DTLS_PEER_H
#define _DTLS_PEER_H

#include <sys/socket.h>
#include <cstddef>
#include <string>
#include <queue>
#include <functional>
#include "eventloop/ip_addr.h"

#define BUFFER_SIZE 4096

using std::string;
using std::queue;

struct ssl_st;
typedef struct ssl_st SSL;
struct ssl_ctx_st;
typedef struct ssl_ctx_st SSL_CTX;

struct sockaddr;

namespace evt_loop {

enum class HandshakeState : uint8_t {
    READY,
    IN_PROCESS,
    DONE,
    FAILED,
};

class DTLSPeer;

class DTLSPeer {

    public:
    using OnPeerReadyCallback = std::function<void (DTLSPeer*)>;
    using OnPeerDisconnectedCallback = std::function<void (DTLSPeer*)>;

    public:
    DTLSPeer(SSL_CTX* ssl_ctx, bool is_server, int sock_fd, const IPAddr* peer_addr);
    ~DTLSPeer();
    void Cleanup();

    size_t SendPacket(const char* data, size_t size, bool need_buffer = true);
    size_t SendPacket(const string& data);

    size_t ReceivePacket(const char* data, size_t size);
    size_t ReceivePacket(const string& data);

    void SetOnReadyCallback(const OnPeerReadyCallback& cb) { on_ready_cb_ = cb; }
    void SetOnDisconnectedCallback(const OnPeerDisconnectedCallback& cb) { on_disconnected_cb_ = cb; }

    const IPAddr* GetPeerAddr() const { return peer_addr_; }

    protected:
    void handle_dtls_error(int result);
    HandshakeState Handshake();
    void OnHandshakeDone();

    private:
    bool is_server_ = false;
    const IPAddr* peer_addr_;
    SSL *ssl_ = nullptr;
    HandshakeState hdshk_state_ = HandshakeState::READY;
    queue<string> tx_buf_queue_;

    OnPeerReadyCallback on_ready_cb_;
    OnPeerDisconnectedCallback on_disconnected_cb_;
};

}  // ns evt_loop

#endif  // _DTLS_PEER_H

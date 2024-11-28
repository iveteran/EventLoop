#include "dtls_peer.h"
#include "eventloop/logger.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace evt_loop {

static SSL* CreateTLSSession(SSL_CTX* ssl_ctx, bool is_server,
        int sock_fd, const struct sockaddr& peer_addr)
{
    // Create new SSL instance for this stream
    SSL* ssl = SSL_new(ssl_ctx);
    if (!ssl) {
        ERR_print_errors_fp(stderr);
        return nullptr;
    }

    // Create BIO for the socket
    BIO* bio = BIO_new_dgram(sock_fd, BIO_NOCLOSE);

    if (is_server) {
        // server side
        SSL_set_options(ssl, SSL_OP_COOKIE_EXCHANGE);
        BIO_ctrl(bio, BIO_CTRL_DGRAM_SET_PEER, 0, (void*)&peer_addr);
    } else {
        // client side
        SSL_set_connect_state(ssl);
        BIO_ctrl(bio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, (void*)&peer_addr);
    }

    /* Set and activate timeouts */
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    BIO_ctrl(bio, BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);

    SSL_set_bio(ssl, bio, bio);

    return ssl;
}

void DTLSPeer::handle_dtls_error(int errcode) {
    //int errcode = SSL_get_error(ssl_, result);
    switch (errcode) {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            // Non-blocking operation, just continue
            break;
        case SSL_ERROR_ZERO_RETURN:
            el_logger->debug("[DTLSPeer] DTLS connection closed");
            if (on_disconnected_cb_) {
                on_disconnected_cb_(this);
            }
            break;
        case SSL_ERROR_SYSCALL:
        case SSL_ERROR_SSL:
            break;
        default:
            ERR_print_errors_fp(stderr);
            break;
    }
}
    
void DTLSPeer::Cleanup()
{
    if (ssl_) {
        // Send close_notify alert and wait for the peer's close_notify
        int ret = SSL_shutdown(ssl_);
        if (ret == 0) {
            // If ret == 0, call SSL_shutdown() again for bidirectional shutdown
            SSL_shutdown(ssl_);
        }
    }
    ssl_ = nullptr;
}

DTLSPeer::DTLSPeer(SSL_CTX* ssl_ctx, bool is_server,
        int sock_fd, const IPAddr* peer_addr)
    : is_server_(is_server), peer_addr_(peer_addr)
{
    ssl_ = CreateTLSSession(ssl_ctx, is_server_, sock_fd, *(peer_addr->SockAddr()));
    assert(ssl_ != nullptr);
}

DTLSPeer::~DTLSPeer()
{
    Cleanup();
}

HandshakeState DTLSPeer::Handshake()
{
    el_logger->debug("[DTLSPeer::Handshake] BEGIN -> handshake state: {}", int(hdshk_state_));
    int result = is_server_ ? SSL_accept(ssl_) : SSL_connect(ssl_);
    if (result == 1) {
        el_logger->debug("[DTLSPeer::Handshake] handshake complete");
        hdshk_state_ = HandshakeState::DONE;
        OnHandshakeDone();
    } else {
        int errcode = SSL_get_error(ssl_, result);
        if (errcode == SSL_ERROR_WANT_WRITE || errcode == SSL_ERROR_WANT_READ) {
            hdshk_state_ = HandshakeState::IN_PROCESS;
        } else {
            el_logger->error("[DTLSPeer::Handshake] error {} msg {}", errcode, strerror(errno));
            ERR_print_errors_fp(stderr);
            hdshk_state_ = HandshakeState::FAILED;
            //OnError(errcode, strerror(errno));
        }
    }
    el_logger->debug("[DTLSPeer::Handshake] END -> handshake state: {}", int(hdshk_state_));
    return hdshk_state_;
}

void DTLSPeer::OnHandshakeDone()
{
    // send buffered packets to peer
    for (; !tx_buf_queue_.empty();) {
        auto packet = tx_buf_queue_.front();
        size_t size = SendPacket(packet);
        if (size > 0) {
            tx_buf_queue_.pop();
        } else {
            break;
        }
    }

    if (on_ready_cb_) {
        on_ready_cb_(this);
    }
}

size_t DTLSPeer::ReceivePacket(const char* buf, size_t bufsize)
{
    if (hdshk_state_ == HandshakeState::READY || hdshk_state_ == HandshakeState::IN_PROCESS) {
        hdshk_state_ = Handshake();
        if (hdshk_state_ != HandshakeState::DONE) {
            return 0;
        }
    }

    int rd = SSL_read(ssl_, (void*)buf, bufsize);
    el_logger->debug("[DTLSPeer::ReceivePacket] size: {}", rd);

    int errcode = SSL_get_error(ssl_, rd);
    if (rd < 0 && errcode != SSL_ERROR_WANT_READ) {
        el_logger->error("SSL_read return {} error {} errno {} msg {}", rd, errcode, errno, strerror(errno));
        ERR_print_errors_fp(stderr);
    }
    handle_dtls_error(errcode);
    return rd < 0 ? 0 : rd;
}

size_t DTLSPeer::ReceivePacket(const string& data)
{
    return ReceivePacket(data.data(), data.size());
}

size_t DTLSPeer::SendPacket(const char* buf, size_t bufsize, bool need_buffer)
{
    if (hdshk_state_ == HandshakeState::READY || hdshk_state_ == HandshakeState::IN_PROCESS) {
        hdshk_state_ = Handshake();
        if (hdshk_state_ != HandshakeState::DONE) {
            if (need_buffer) {
                tx_buf_queue_.push(string(buf, bufsize));
            }
            return 0;
        }
    }

    int wd = SSL_write(ssl_, buf, bufsize);
    el_logger->debug("[DTLSPeer::SendPacket] size: {}", wd);

    int errcode = SSL_get_error(ssl_, wd);
    if (wd < 0 && errcode != SSL_ERROR_WANT_WRITE) {
        el_logger->error("SSL_write return {} error {} errno {} msg {}", wd, errcode, errno, strerror(errno));
        ERR_print_errors_fp(stderr);
    }
    handle_dtls_error(errcode);
    return wd < 0 ? 0 : wd;
}

size_t DTLSPeer::SendPacket(const string& data)
{
    auto tx_size = SendPacket(data.data(), data.size(), false);
    if (tx_size == 0) {
        tx_buf_queue_.push(data);
    }
    return tx_size;
}

}  // ns evt_loop

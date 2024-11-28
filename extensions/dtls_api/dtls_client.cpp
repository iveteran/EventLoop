#include "dtls_client.h"
#include "dtls_peer.h"
#include "ssl_setup.h"
#include "eventloop/logger.h"

namespace evt_loop {

static SSL_CTX* ssl_ctx_;

DTLSClient::DTLSClient(IPVer ip_ver, const char* host, uint16_t port)
    : UdpPeer(ip_ver, host, port, UdpPeer::Mode::CLIENT)
{
}

DTLSClient::~DTLSClient()
{
    if (peer_) {
        delete peer_;
        peer_ = nullptr;
    }
    //EVP_cleanup();
}

bool DTLSClient::Init(const char* cert_file, const char* key_file, const char* ca_file)
{
    if (! ssl_ctx_) {
        const bool is_server = false;
        ssl_ctx_ = SetupSSLContext(is_server, cert_file, key_file, ca_file);
    }
    assert(ssl_ctx_ != nullptr);

    return true;
}

bool DTLSClient::Connect()
{
    if (! UdpPeer::Connect()) {
        el_logger->error("Connect failed");
        return false;
    }

    bool is_server = false;
    peer_ = new DTLSPeer(ssl_ctx_, is_server, remote_peer_addr_, FD());
    peer_->SetOnReadyCallback(on_peer_ready_cb_);
    peer_->SetOnDisconnectedCallback(on_peer_disconnected_cb_);
    return true;
}

size_t DTLSClient::_receive_packet(const char* buf, size_t bufsize,
        int flags, IPAddr* peer_addr)
{
    return peer_->ReceivePacket(buf, bufsize);
}

size_t DTLSClient::_send_packet(const char* data, size_t size,
        int flags, const IPAddr* peer_addr)
{
    return peer_->SendPacket(data, size);
}

}  // ns evt_loop

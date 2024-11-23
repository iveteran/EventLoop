#ifndef _DTLS_CLIENT_H
#define _DTLS_CLIENT_H

#include "eventloop/udp_peer.h"
#include "dtls_peer.h"

namespace evt_loop {

class DTLSPeer;

class DTLSClient : public UdpPeer {
    public:
    DTLSClient(IPVer ip_ver, const char* host, uint16_t port);
    ~DTLSClient();

    bool Init(const char* cert_file, const char* key_file, const char* ca_file);
    void Destroy();
    bool Connect();

    void SetOnPeerDisconnectedCallback(const OnPeerDisconnectedCallback& cb) { on_peer_disconnected_cb_ = cb; }
    void SetOnPeerReadyCallback(const OnPeerReadyCallback& cb) { on_peer_ready_cb_ = cb; }

    protected:
    size_t _receive_packet(const char* buf, size_t bufsize,
            int flags, IPAddr* peer_addr) override;
    size_t _send_packet(const char* data, size_t size,
            int flags, const IPAddr* peer_addr) override;

    private:
    DTLSPeer* peer_ = nullptr;

    OnPeerReadyCallback on_peer_ready_cb_;
    OnPeerDisconnectedCallback on_peer_disconnected_cb_;
};

}  // ns evt_loop

#endif  // _DTLS_CLIENT_H

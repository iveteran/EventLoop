#ifndef _DTLS_SERVER_H
#define _DTLS_SERVER_H

#include "eventloop/udp_peer.h"
#include "dtls_peer.h"

#include <string>
#include <map>

using std::string;
using std::map;

struct sockaddr_in;

namespace evt_loop {

//class DTLSPeer;

using OnPeerConnectedCallback = std::function<void (DTLSPeer*)>;

class DTLSServer : public UdpPeer {
    public:
    DTLSServer(IPVer ip_ver, const char* host, uint16_t port);
    virtual ~DTLSServer();

    //void Init(IPVer ip_ver, const char* host, uint16_t port);
    void Init(const char* cert_file, const char* key_file, const char* ca_file = nullptr);
    void Destroy();

    void SetOnPeerConnectedCallback(const OnPeerConnectedCallback& cb) { on_peer_connected_cb_ = cb; }
    void SetOnPeerDisconnectedCallback(const OnPeerDisconnectedCallback& cb) { on_peer_disconnected_cb_ = cb; }
    void SetOnPeerReadyCallback(const OnPeerReadyCallback& cb) { on_peer_ready_cb_ = cb; }

    protected:
    void OnPeerConnected(DTLSPeer* peer);
    void OnPeerDisconnected(DTLSPeer* peer);
    bool ObtainPeerAddress(struct sockaddr* peer_addr);

    size_t _send_packet(const char* data, size_t size, int flags,
            const IPAddr* peer_addr) override;

    size_t _receive_packet(const char* buf, size_t bufsize, int flags,
            IPAddr* peer_addr) override;

    private:
    std::map<string, DTLSPeer*> peers_;

    OnPeerConnectedCallback on_peer_connected_cb_;
    OnPeerReadyCallback on_peer_ready_cb_;
    OnPeerDisconnectedCallback on_peer_disconnected_cb_;
};

}  // ns evt_loop

#endif  // _DTLS_SERVER_H

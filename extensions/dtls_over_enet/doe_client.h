#ifndef _DOE_CLIENT_H
#define _DOE_CLIENT_H

#include "../dtls_api/dtls_peer.h"
#include "../enet/enet_client.h"

#include <string>

using std::string;

// forward declaration
struct _ENetPeer;
typedef _ENetPeer ENetPeer;

namespace evt_loop {

class DOEPeer;

// DTLS over Enet client
class DOEClient {
    public:
    using OnPeerConnectedCallback = std::function<void (DOEPeer*)>;
    using OnPeerDisconnectedCallback = std::function<void (DOEPeer*)>;
    using OnPeerReadyCallback = std::function<void (DOEPeer*)>;
    using OnPacketReceivedCallback = std::function<void (DOEPeer*, const char* data, size_t size)>;
    using OnErrorCallback = std::function<void (DOEPeer*, int errcode, const char* errmsg)>;

    public:
    DOEClient();
    virtual ~DOEClient();

    //void Init(IPVer ip_ver, const char* host, uint16_t port);
    void Init(const char* cert_file, const char* key_file, const char* ca_file = nullptr);
    void Destroy();
    bool Connect(IPVer ip_ver, const char* host, uint16_t port);
    void Disconnect();

    void SetOnPeerConnectedCallback(const OnPeerConnectedCallback& cb) { on_peer_connected_cb_ = cb; }
    void SetOnPeerDisconnectedCallback(const OnPeerDisconnectedCallback& cb) { on_peer_disconnected_cb_ = cb; }
    void SetOnPeerReadyCallback(const OnPeerReadyCallback& cb) { on_peer_ready_cb_ = cb; }
    void SetOnPacketReceivedCallback(const OnPacketReceivedCallback& cb) { on_packet_received_cb_ = cb; }
    void SetOnErrorCallback(const OnErrorCallback& cb) { on_error_cb_ = cb; }

    protected:
    void OnENetPeerConnected(ENetPeer* enet_peer);
    void OnENetPeerDisconnected(ENetPeer* enet_peer);
    void OnENetPeerReceivedPacket(ENetPeer* enet_peer, const char* data, size_t size);

    void OnPeerConnected(DOEPeer* peer);
    void OnPeerDisconnected(DOEPeer* peer);

    void OnDTLSPeerDisconnected(DTLSPeer* dtls_peer);
    void OnDTLSPeerReady(DTLSPeer* dtls_peer);

    private:
    ENetClient  enet_client_;
    DOEPeer*    peer_ = nullptr;

    OnPeerConnectedCallback     on_peer_connected_cb_;
    OnPeerDisconnectedCallback  on_peer_disconnected_cb_;
    OnPeerReadyCallback         on_peer_ready_cb_;
    OnPacketReceivedCallback    on_packet_received_cb_;
    OnErrorCallback             on_error_cb_;
};

}  // ns evt_loop

#endif  // _DOE_CLIENT_H

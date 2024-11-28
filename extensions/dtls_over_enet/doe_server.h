#ifndef _DOE_SERVER_H
#define _DOE_SERVER_H

#include "../dtls_api/dtls_peer.h"
#include "../enet/enet_server.h"

#include <string>
#include <map>

using std::string;
using std::map;

// forward declaration
struct _ENetPeer;
typedef _ENetPeer ENetPeer;

namespace evt_loop {

class DOEPeer;

// DTLS over Enet server
class DOEServer {
    public:
    using OnPeerConnectedCallback = std::function<void (DOEPeer*)>;
    using OnPeerDisconnectedCallback = std::function<void (DOEPeer*)>;
    using OnPeerReadyCallback = std::function<void (DOEPeer*)>;
    using OnPacketReceivedCallback = std::function<void (DOEPeer*, const char* data, size_t size)>;
    using OnErrorCallback = std::function<void (DOEPeer*, int errcode, const char* errmsg)>;

    public:
    DOEServer(IPVer ip_ver, uint16_t port, const char* host = nullptr);
    virtual ~DOEServer();

    //void Init(IPVer ip_ver, const char* host, uint16_t port);
    void Init(const char* cert_file, const char* key_file, const char* ca_file = nullptr);
    void Destroy();

    void SetOnPeerConnectedCallback(const OnPeerConnectedCallback& cb) { on_peer_connected_cb_ = cb; }
    void SetOnPeerDisconnectedCallback(const OnPeerDisconnectedCallback& cb) { on_peer_disconnected_cb_ = cb; }
    void SetOnPeerReadyCallback(const OnPeerReadyCallback& cb) { on_peer_ready_cb_ = cb; }
    void SetOnPacketReceivedCallback(const OnPacketReceivedCallback& cb) { on_packet_received_cb_ = cb; }
    void SetOnErrorCallback(const OnErrorCallback& cb) { on_error_cb_ = cb; }

    protected:
    void OnEnetNewClient(ENetPeer* enet_peer);
    void OnENetPeerDisconnected(ENetPeer* enet_peer);
    void OnENetReceivedPacket(ENetPeer* enet_peer, const char* data, size_t size);

    void OnPeerConnected(DOEPeer* peer);
    void OnPeerDisconnected(DOEPeer* peer);

    void OnDTLSPeerDisconnected(DTLSPeer* dtls_peer);
    void OnDTLSPeerReady(DTLSPeer* dtls_peer);

    private:
    ENetServer enet_server_;
    std::map<string, DOEPeer*> peers_;

    OnPeerConnectedCallback     on_peer_connected_cb_;
    OnPeerDisconnectedCallback  on_peer_disconnected_cb_;
    OnPeerReadyCallback         on_peer_ready_cb_;
    OnPacketReceivedCallback    on_packet_received_cb_;
    OnErrorCallback             on_error_cb_;
};

}  // ns evt_loop

#endif  // _DOE_SERVER_H

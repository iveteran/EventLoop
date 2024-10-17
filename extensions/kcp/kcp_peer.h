#ifndef _KCP_PEER_H
#define _KCP_PEER_H

#include "eventloop/udp_peer.h"
#include "eventloop/el.h"
#include <functional>

typedef struct IKCPCB ikcpcb;

namespace evt_loop {

class UdpPeer4;
class UdpPeer;
class PeerAddr;

int udp_output(const char *buf, int len, ikcpcb *kcp, void *userdata);

class KcpPeer
{
    public:
    using OnKcpPacketCallback = std::function<void (KcpPeer*, const char* data, size_t size)>;

    KcpPeer(uint32_t sess_id, uint16_t peer_id);
    ~KcpPeer();

    void InitUdpPeer(const char* host, uint16_t port, UdpPeer::Mode mode);
    bool BindUdpAddr(const char* ip, uint16_t port);
    void SetKcpParameters();
    void SetOnPacketCallback(const OnKcpPacketCallback& cb) { on_packet_cb_ = cb; }
    void SendPacket(const char* data, size_t size);
    void ReceivePacket();

    uint32_t GetSessionId() const { return kcp_sess_id_; }
    uint16_t GetPeerId() const { return kcp_peer_id_; }

    protected:
    void OnUdpError(UdpPeer* udp_peer, int errcode, const char* errmsg);
    void OnUdpPacket(UdpPeer* udp_peer, const PeerAddr* remote_addr, const char* data, size_t size);
    void SendUdpPacket(const char* data, size_t size);
    void OnTick(UserEvent* tick_event, void* udata);

    private:
    uint32_t kcp_sess_id_;
    uint16_t kcp_peer_id_;
    ikcpcb* kcp_;
    OnKcpPacketCallback on_packet_cb_;

    UdpPeer4 udp_peer_;
    const PeerAddr* remote_addr_;
    TickEvent tick_task_;

    int stats_interval_;
    int stats_count_;
    int stats_sum_rtt_;
    int stats_max_rtt_;

    friend int udp_output(const char *buf, int len, ikcpcb *kcp, void *userdata);
};

}  // ns evt_loop

#endif  // _KCP_PEER_H

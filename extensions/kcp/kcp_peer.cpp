#include "kcp_peer.h"
#include "ikcp.h"
#include "eventloop/udp_peer.h"

namespace evt_loop {

#pragma pack(1)
struct kcp_hdr
{
    uint32_t conv;
    uint8_t  cmd;
    uint8_t  frg;
    uint16_t wnd;
    uint32_t ts;
    uint32_t sn;
    uint32_t una;
    uint32_t len;
    char     data[0];
};
#pragma pack()

int udp_output(const char *buf, int len, ikcpcb *kcp, void *userdata)
{
    auto kcp_peer = (KcpPeer*)userdata;
    kcp_peer->SendUdpPacket(buf, len);
    return 0;
}

KcpPeer::KcpPeer(uint32_t sess_id, uint16_t peer_id)
    : kcp_sess_id_(sess_id), kcp_peer_id_(peer_id)
    , tick_task_(std::bind(&KcpPeer::OnTick, this, std::placeholders::_1, std::placeholders::_2))
{
    kcp_ = ikcp_create(kcp_sess_id_, (void*)this);
    kcp_->output = udp_output;

    SetKcpParameters();

    udp_peer_.SetOnPacketCallback(std::bind(&KcpPeer::OnUdpPacket, this,
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    udp_peer_.SetOnErrorCallback(std::bind(&KcpPeer::OnUdpError, this,
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}
KcpPeer::~KcpPeer()
{
    ikcp_release(kcp_);
}

void KcpPeer::InitUdpPeer(const char* host, uint16_t port, UdpPeer::Mode mode)
{
    udp_peer_.Init(host, port, mode);
}

bool KcpPeer::BindUdpAddr(const char* ip, uint16_t port)
{
    return udp_peer_.Bind(ip, port);
}

void KcpPeer::SetKcpParameters()
{
    int tick_ms = EV_Singleton->GetTickMS();
    ikcp_wndsize(kcp_, 128, 128);
    ikcp_nodelay(kcp_, 0, tick_ms, 0, 0);  // default mode
}

void KcpPeer::SendPacket(const char* data, size_t size)
{
    ikcp_send(kcp_, data, size);
}

void KcpPeer::OnUdpError(UdpPeer* udp_peer, int errcode, const char* errmsg)
{
    el_logger->debug("[KcpPeer::OnUdpError] peer id: {}, error: {}", kcp_peer_id_, errmsg);
}

void KcpPeer::OnUdpPacket(UdpPeer* udp_peer, const PeerAddr* remote_addr, const char* data, size_t size)
{
    remote_addr_ = remote_addr;
    el_logger->debug("[KcpPeer::OnUdpPacket] peer addr: {}, peer id: {}, received data size: {}",
            remote_addr->String(), kcp_peer_id_, size);
    el_logger->output(DumpHexWithChars(data, size, 0)).eol();
    ikcp_input(kcp_, data, size);
}

void KcpPeer::SendUdpPacket(const char* data, size_t size)
{
    el_logger->debug("[KcpPeer::SendUdpPacket] peer id: {}, send data size: {}", kcp_peer_id_, size);
    el_logger->output(DumpHexWithChars(data, size, 0)).eol();
    if (remote_addr_) {
        el_logger->debug("[KcpPeer::SendUdpPacket] peer addr: {}", remote_addr_->String());
        udp_peer_.SendPacket(remote_addr_, data, size);
    } else {
        udp_peer_.SendPacket(data, size);
    }
}

void KcpPeer::OnTick(UserEvent* tick_event, void* udata)
{
    //el_logger->debug("[OnTick] Trigger tick event(id: {}), udata: {}", tick_event->Id(), udata);
    int64_t now_ms = NowMilliSeconds();
    uint32_t clock = (uint32_t)(now_ms & 0xfffffffful);
    //el_logger->debug("[OnTick] now clock: {}, {}, {}", now_ms, clock);
    ikcp_update(kcp_, clock);
    ReceivePacket();
}

void KcpPeer::ReceivePacket()
{
    char buffer[2048];
    int size = ikcp_recv(kcp_, buffer, sizeof(buffer));
    if (size < 0) {
        // have no packet
        return;
    }
    /*
    int hr = ikcp_recv(kcp_, buffer, 10);
    // 没有收到包就退出
    if (hr < 0) return;

    IUINT32 sn = *(IUINT32*)(buffer + 0);
    IUINT32 ts = *(IUINT32*)(buffer + 4);
    IUINT32 rtt = current - ts;

    if (sn != next) {
        // 如果收到的包不连续
        el_logger->error("ERROR sn {}<->{}", (int)count, (int)next);
        return;
    }

    next++;
    sumrtt += rtt;
    count++;
    if (rtt > (IUINT32)maxrtt) maxrtt = rtt;

    el_logger->debug("[RECV] mode={} sn={} rtt={}", mode, (int)sn, (int)rtt);
    */

    el_logger->debug("[KcpPeer::ReceivePacket] peer id: {}, received data size: {}", kcp_peer_id_, size);
    el_logger->output(DumpHexWithChars(buffer, size, 0)).eol();
    if (on_packet_cb_) {
        on_packet_cb_(this, buffer, size);
    }
}

}  // ns evt_loop

#ifndef _UDP_PEER_H
#define _UDP_PEER_H

#include "io_event.h"
#include "ip_addr.h"
#include <memory>

namespace evt_loop {

#ifdef USE_IO_URING
class URingRecvPacketRequest;
#endif

class UdpPeer: public IOEvent
{
    public:
    enum class Mode : uint8_t {
        LOCAL  = 0,
        SERVER = 0,
        REMOTE = 1,
        CLIENT = 1,
    };
    using OnConnectedCallback = std::function<void (UdpPeer*)>;
    using OnDisconnectedCallback = std::function<void (UdpPeer*)>;
    using OnErrorCallback = std::function<void (UdpPeer*, int, const char*)>;
    using OnPacketCallback = std::function<void (UdpPeer*, const IPAddr*, const char*, size_t)>;

    UdpPeer() : IOEvent(IOType::UDP_PEER) {}
    UdpPeer(IPVer ip_ver, const char *host, uint16_t port, Mode mode);
    virtual ~UdpPeer();
    void Init(IPVer ip_ver, const char *host, uint16_t port, Mode mode);
    void Destroy();

    IPVer GetIPVersion() const { return ip_ver_; }
    const IPAddress& GetAddress() const { return ip_addr_; }

    bool Bind(const char* ip, uint16_t port, int fd = -1);
    bool Connect();

    const IPAddr* PeerLocalAddr() const { return local_peer_addr_; }
    const IPAddr* PeerRemoteAddr() const { return remote_peer_addr_; }

    void SetOnPacketCallback(const OnPacketCallback& cb) { on_packet_cb_ = cb; }
    void SetOnErrorCallback(const OnErrorCallback& cb) { on_error_cb_ = cb; }

    size_t SendPacket(const char* data, size_t size);
    size_t SendPacket(const IPAddr* peer_addr, const char* data, size_t size);

    protected:
    void InitAddress(const char* host, uint16_t port);
    void InitAddress6(const char* host, uint16_t port);
    bool Create(Mode mode);

    void OnError(int errcode, const char* errstr) override;
    void OnEvents(uint32_t events, void* ctx = nullptr) override;

    size_t ReceivePacket();
    size_t ReceivePacket(char* recvbuf, size_t recvbuf_size);
    //template<typename T>
    //    size_t ReceivePacket(char* recvbuf, size_t recvbuf_size);

    virtual size_t _send_packet(const char* data, size_t size, int flags, const IPAddr* peer_addr)
    {
        return sendto(fd_, data, size, flags, peer_addr->SockAddr(), peer_addr->Size());
    }

    virtual size_t _receive_packet(const char* buf, size_t bufsize, int flags, IPAddr* peer_addr)
    {
        socklen_t peer_addr_size = peer_addr->Size();
        return recvfrom(fd_, (void*)buf, bufsize, flags, (struct sockaddr*)peer_addr->SockAddr(), &peer_addr_size);
    }

#ifdef USE_IO_URING
    size_t URingReceivePacket(URingRecvPacketRequest* req);
#endif

    protected:
    IPVer ip_ver_;
    IPAddress ip_addr_;
    IPAddr*  local_peer_addr_ = nullptr;
    IPAddr*  remote_peer_addr_ = nullptr;

    OnConnectedCallback     on_connected_cb_;
    OnDisconnectedCallback  on_disconnected_cb_;
    OnErrorCallback     on_error_cb_;
    OnPacketCallback    on_packet_cb_;
};
typedef std::shared_ptr<UdpPeer> UdpPeerPtr;

}  // namespace evt_loop

#endif  // _UDP_PEER_H

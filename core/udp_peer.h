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
        LOCAL,
        REMOTE
    };
    using OnErrorCallback = std::function<void (UdpPeer*, int, const char*)>;
    using OnPacketCallback = std::function<void (UdpPeer*, const IPAddr*, const char*, size_t)>;

    UdpPeer() : IOEvent(IOType::UDP_PEER) {}
    UdpPeer(IPVer ip_ver, const char *host, uint16_t port, Mode mode);
    ~UdpPeer();
    void Init(IPVer ip_ver, const char *host, uint16_t port, Mode mode);
    void Destroy();

    IPVer GetIPVersion() const { return ip_ver_; }
    const IPAddress& GetAddress() const { return ip_addr_; }

    void SetOnPacketCallback(const OnPacketCallback& cb) { on_packet_cb_ = cb; }
    void SetOnErrorCallback(const OnErrorCallback& cb) { on_error_cb_ = cb; }

    size_t SendPacket(const char* data, size_t size);
    size_t SendPacket(const IPAddr* peer_addr, const char* data, size_t size);

    protected:
    void InitAddress(const char* host, uint16_t port);
    void InitAddress6(const char* host, uint16_t port);
    bool Create(Mode mode);
    bool Bind(const char* ip, uint16_t port, int fd = -1);

    const IPAddr* PeerLocalAddr() const { return local_peer_addr_; }
    const IPAddr* PeerRemoteAddr() const { return remote_peer_addr_; }

    void OnError(int errcode, const char* errstr) override;
    void OnEvents(uint32_t events, void* ctx = nullptr) override;

    size_t ReceivePacket();
    size_t ReceivePacket(char* recvbuf, size_t recvbuf_size);
    //template<typename T>
    //    size_t ReceivePacket(char* recvbuf, size_t recvbuf_size);

#ifdef USE_IO_URING
    size_t URingReceivePacket(URingRecvPacketRequest* req);
#endif

    protected:
    IPVer ip_ver_;
    IPAddress ip_addr_;
    IPAddr*  local_peer_addr_;
    IPAddr*  remote_peer_addr_;

    OnErrorCallback     on_error_cb_;
    OnPacketCallback    on_packet_cb_;
};
typedef std::shared_ptr<UdpPeer> UdpPeerPtr;

}  // namespace evt_loop

#endif  // _UDP_PEER_H

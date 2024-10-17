#ifndef _UDP_PEER_H
#define _UDP_PEER_H

#include "io_event.h"
#include "peer_addr.h"
#include "utils.h"
#include <memory>

namespace evt_loop {

class UdpPeer: public IOEvent
{
    public:
    enum class Mode : uint8_t {
        LOCAL,
        REMOTE
    };
    using OnErrorCallback = std::function<void (UdpPeer*, int, const char*)>;
    using OnPacketCallback = std::function<void (UdpPeer*, const PeerAddr*, const char*, size_t)>;

    //UdpPeer(const char *host, uint16_t port, Mode mode);
    ~UdpPeer();
    void Init(const char *host, uint16_t port, Mode mode);
    void Destroy();

    const IPAddress& GetAddress() const { return ip_addr_; }

    void SetOnPacketCallback(const OnPacketCallback& cb) { on_packet_cb_ = cb; }
    void SetOnErrorCallback(const OnErrorCallback& cb) { on_error_cb_ = cb; }

    size_t SendPacket(const char* data, size_t size);
    size_t SendPacket(const PeerAddr* peer_addr, const char* data, size_t size);

    protected:
    virtual void InitAddress(const char* host, uint16_t port) = 0;
    virtual bool Create(Mode mode) = 0;

    virtual const PeerAddr* PeerLocalAddr() const = 0;
    virtual const PeerAddr* PeerRemoteAddr() const = 0;

    void OnError(int errcode, const char* errstr);
    void OnEvents(uint32_t events);

    size_t ReceivePacket();
    virtual size_t ReceivePacket(char* recvbuf, size_t recvbuf_size) = 0;
    //template<typename T>
    //    size_t ReceivePacket(char* recvbuf, size_t recvbuf_size);

    protected:
    IPAddress ip_addr_;

    OnErrorCallback     on_error_cb_;
    OnPacketCallback    on_packet_cb_;
};
typedef std::shared_ptr<UdpPeer> UdpPeerPtr;

class UdpPeer4: public UdpPeer
{
    public:
    //UdpPeer4(const char *host, uint16_t port, Mode mode);
    bool Bind(const char* ip, uint16_t port);

    protected:
    virtual void InitAddress(const char* host, uint16_t port) override;
    virtual bool Create(Mode mode) override;
    const PeerAddr* PeerLocalAddr() const { return &local_peer_addr_; }
    const PeerAddr* PeerRemoteAddr() const { return &remote_peer_addr_; }

    size_t ReceivePacket(char* recvbuf, size_t recvbuf_size);

    private:
    PeerAddr4  local_peer_addr_;
    PeerAddr4  remote_peer_addr_;
};

class UdpPeer6: public UdpPeer
{
    public:
    UdpPeer6(bool ipv6_only = true) : ipv6_only_(ipv6_only) {}
    //UdpPeer6(const char *host, uint16_t port, Mode mode, bool ipv6_only = true);
    bool Bind(const char* ip, uint16_t port);

    protected:
    virtual void InitAddress(const char* host, uint16_t port) override;
    virtual bool Create(Mode mode) override;
    const PeerAddr* PeerLocalAddr() const { return &local_peer_addr_; }
    const PeerAddr* PeerRemoteAddr() const { return &remote_peer_addr_; }

    size_t ReceivePacket(char* recvbuf, size_t recvbuf_size);

    private:
    bool ipv6_only_;

    PeerAddr6  local_peer_addr_;
    PeerAddr6  remote_peer_addr_;
};
typedef std::shared_ptr<UdpPeer6> UdpPeer6Ptr;

}  // namespace evt_loop

#endif  // _UDP_PEER_H

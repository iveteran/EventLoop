#include "udp_peer.h"
#include "logger.h"

#define UDP_MAX_BYTES 65535

namespace evt_loop {

//UdpPeer::UdpPeer(const char *host, uint16_t port, Mode mode)
//    : IOEvent(IOType::UDP_PEER)
//{
//    //Init(host, port, mode);
//}

UdpPeer::~UdpPeer()
{
    Destroy();
}

void UdpPeer::Init(const char *host, uint16_t port, Mode mode)
{
    InitAddress(host, port);
    Create(mode);
}

void UdpPeer::Destroy()
{
    close(fd_);
    SetFD(-1);
}

void UdpPeer::OnEvents(uint32_t events, void* ctx)
{
    if (events & FileEvent::READ) {
        size_t size = ReceivePacket();
        if (size < 0) {
            events |= FileEvent::ERROR;
        }
    }
    if (events & FileEvent::WRITE_DONE) {
        size_t tx_bytes = 0;
        OnWriteDone(tx_bytes);
    }

    if (events & FileEvent::ERROR) {
        OnError(errno, strerror(errno));
    }
}

size_t UdpPeer::ReceivePacket()
{
    char recvbuf[UDP_MAX_BYTES];
    return ReceivePacket(recvbuf, sizeof(recvbuf));
}

/*
template<typename T>
size_t UdpPeer::ReceivePacket(char* recvbuf, size_t recvbuf_size)
{
    T remote_peer_addr;
    socklen_t sock_addr_size = remote_peer_addr.Size();
    size_t rx_bytes = recvfrom(fd_, recvbuf, recvbuf_size, 0, (struct sockaddr*)remote_peer_addr.SockAddr(), &sock_addr_size);
    el_logger->debug("[UdpPeer::ReceivePacket] client: {}, bytes size: {}", remote_peer_addr.String(), rx_bytes);
    if (rx_bytes > 0 && on_packet_cb_) {
        on_packet_cb_(this, &remote_peer_addr, recvbuf, rx_bytes);
    }
    return rx_bytes;
}
*/

size_t UdpPeer::SendPacket(const char* data, size_t size)
{
    return SendPacket(PeerRemoteAddr(), data, size);
}

size_t UdpPeer::SendPacket(const PeerAddr* peer_addr, const char* data, size_t size)
{
    el_logger->debug("[UdpPeer::SendPacket] sendto: {}, size: {}", peer_addr->String(), size);
    return sendto(fd_, data, size, 0, peer_addr->SockAddr(), peer_addr->Size());
}

void UdpPeer::OnError(int errcode, const char* errstr)
{
    el_logger->error("[UdpPeer::OnError] error code: {}, error string: {}", errcode, errstr);
    if (on_error_cb_) on_error_cb_(this, errcode, errstr);

    if (errcode == EADDRINUSE || errcode == EADDRNOTAVAIL) {
        exit(1);
    }
}

//////////////////////////////////////////////

//UdpPeer4::UdpPeer4(const char *host, uint16_t port, Mode mode)
//    : UdpPeer(host, port, mode)
//{
//}

void UdpPeer4::InitAddress(const char* host, uint16_t port)
{
    ip_addr_.port_ = port;
    if (host[0] == '\0' || strcmp(host, "localhost") == 0) {
        ip_addr_.ip_ = "127.0.0.1";
    } else if (strcmp(host, "any") == 0) {
        ip_addr_.ip_ = "0.0.0.0";
    } else {
        ip_addr_.ip_ = host;
    }
}

bool UdpPeer4::Create(Mode mode)
{
    int fd = -1;
    if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        OnError(errno, strerror(errno));
        return false;
    }

    int reuseaddr = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1)
    {
        OnError(errno, strerror(errno));
        close(fd);
        return false;
    }
    SetFD(fd);

    if (mode == Mode::LOCAL) {
        Bind(ip_addr_.ip_.c_str(), ip_addr_.port_);
    } else {
        remote_peer_addr_ = PeerAddr4(ip_addr_.ip_.c_str(), ip_addr_.port_);
        el_logger->info("[UdpPeer4::Create] Remote address: {}", remote_peer_addr_.String());
    }

    return true;
}

bool UdpPeer4::Bind(const char* ip, uint16_t port)
{
    local_peer_addr_ = PeerAddr4(ip, port);
    el_logger->info("[UdpPeer4::Bind] Bind to address: {}", local_peer_addr_.String());

    if (::bind(fd_, (struct sockaddr*)local_peer_addr_.SockAddr(), local_peer_addr_.Size()) == -1) {
        el_logger->error("[UdpPeer4::Bind] Bind to address failed: {}", strerror(errno));
        return false;
    }
    return true;
}

size_t UdpPeer4::ReceivePacket(char* recvbuf, size_t recvbuf_size)
{
    PeerAddr4 remote_peer_addr;
    socklen_t sock_addr_size = remote_peer_addr.Size();
    size_t rx_bytes = recvfrom(fd_, recvbuf, recvbuf_size, 0, (struct sockaddr*)remote_peer_addr.SockAddr(), &sock_addr_size);
    el_logger->debug("[UdpPeer4::ReceivePacket] client: {}, bytes size: {}", remote_peer_addr.String(), rx_bytes);
    if (rx_bytes > 0 && on_packet_cb_) {
        if (remote_peer_addr_.Port() == 0) {
            // set remote peer addr with currently
            remote_peer_addr_ = remote_peer_addr;
        }
        on_packet_cb_(this, &remote_peer_addr, recvbuf, rx_bytes);
    }
    return rx_bytes;
}

//////////////////////////////////////////////

//UdpPeer6::UdpPeer6(const char *host, uint16_t port, Mode mode, bool ipv6_only)
//    : UdpPeer(host, port, mode), ipv6_only_(ipv6_only)
//{
//}

void UdpPeer6::InitAddress(const char* host, uint16_t port)
{
    ip_addr_.port_ = port;
    if (host[0] == '\0' ||
            strcmp(host, "localhost6") == 0 ||
            strcmp(host, "ip6-localhost") == 0 ||
            strcmp(host, "ipv6-localhost") == 0) {
        ip_addr_.ip_ = "::1";
    } else if (strcmp(host, "any") == 0) {
        ip_addr_.ip_ = "::";
    } else {
        ip_addr_.ip_ = host;
    }
}

bool UdpPeer6::Create(Mode mode)
{
    int fd = -1;
    if ((fd = socket(PF_INET6, SOCK_DGRAM, 0)) == -1) {
        OnError(errno, strerror(errno));
        return false;
    }

    int reuseaddr = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1)
    {
        OnError(errno, strerror(errno));
        close(fd);
        return false;
    }

    if (ipv6_only_) {
        int on = 1;
        if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) == -1) {
            OnError(errno, strerror(errno));
            return false;
        }
    }
    SetFD(fd);

    if (mode == Mode::LOCAL) {
        Bind(ip_addr_.ip_.c_str(), ip_addr_.port_);
    } else {
        remote_peer_addr_ = PeerAddr6(ip_addr_.ip_.c_str(), ip_addr_.port_);
        //remote_peer_addr_.Assign(ip_addr_.ip_.c_str(), ip_addr_.port_);
        el_logger->info("[UdpPeer6::Create] Remote address: {}", remote_peer_addr_.String());
    }

    return true;
}

bool UdpPeer6::Bind(const char* ip, uint16_t port)
{
    local_peer_addr_ = PeerAddr6(ip, port);
    //local_peer_addr_.Assign(ip, port);
    el_logger->info("[UdpPeer6::Bind] Bind to address: {}", local_peer_addr_.String());

    if (::bind(fd_, (struct sockaddr*)local_peer_addr_.SockAddr(), local_peer_addr_.Size()) == -1) {
        el_logger->error("[UdpPeer6::Bind] Bind to address failed: {}", strerror(errno));
        return false;
    }
    return true;
}

size_t UdpPeer6::ReceivePacket(char* recvbuf, size_t recvbuf_size)
{
    PeerAddr6 remote_peer_addr;
    socklen_t sock_addr_size = remote_peer_addr.Size();
    size_t rx_bytes = recvfrom(fd_, recvbuf, recvbuf_size, 0, (struct sockaddr*)remote_peer_addr.SockAddr(), &sock_addr_size);
    el_logger->debug("[UdpPeer6::ReceivePacket] client: {}, bytes size: {}", remote_peer_addr.String(), rx_bytes);
    if (rx_bytes > 0 && on_packet_cb_) {
        on_packet_cb_(this, &remote_peer_addr, recvbuf, rx_bytes);
    }
    return rx_bytes;
}

}  // namespace evt_loop

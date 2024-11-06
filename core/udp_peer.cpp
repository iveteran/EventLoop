#include "udp_peer.h"
#include "logger.h"

#define UDP_MAX_BYTES 65535

namespace evt_loop {

UdpPeer::UdpPeer(IPVer ip_ver, const char *host, uint16_t port, Mode mode)
    : IOEvent(IOType::UDP_PEER)
{
    Init(ip_ver, host, port, mode);
}

UdpPeer::~UdpPeer()
{
    Destroy();
}

void UdpPeer::Init(IPVer ip_ver, const char *host, uint16_t port, Mode mode)
{
    ip_ver_ = ip_ver;
    if (ip_ver_ == IPVer::V4) {
        InitAddress(host, port);
    } else {
        InitAddress6(host, port);
    }
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
        size_t tx_bytes = ctx ? *(int*)ctx : 0;
        el_logger->debug("[UdpPeer::OnEvents.WRITE_DONE] tx_bytes: {}", tx_bytes);
        OnWriteDone(tx_bytes);
    }

    if (events & FileEvent::ERROR) {
        OnError(errno, strerror(errno));
    }
}

void UdpPeer::InitAddress(const char* host, uint16_t port)
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

void UdpPeer::InitAddress6(const char* host, uint16_t port)
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

bool UdpPeer::Create(Mode mode)
{
    int fd = -1;
    int domain = (ip_ver_ == IPVer::V4 ? PF_INET : PF_INET6);
    if ((fd = socket(domain, SOCK_DGRAM, 0)) == -1) {
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

    if (ip_ver_ == IPVer::V6_ONLY) {
        int on = 1;
        if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) == -1) {
            OnError(errno, strerror(errno));
            return false;
        }
    }

    if (mode == Mode::LOCAL) {
        Bind(ip_addr_.ip_.c_str(), ip_addr_.port_);
    } else {
        if (ip_ver_ == IPVer::V4) {
            remote_peer_addr_ = new IPAddr4(ip_addr_.ip_.c_str(), ip_addr_.port_);
        } else {
            remote_peer_addr_ = new IPAddr6(ip_addr_.ip_.c_str(), ip_addr_.port_);
        }
        el_logger->info("[UdpPeer::Create] Remote address: {}", remote_peer_addr_->String());
    }

    return true;
}

bool UdpPeer::Bind(const char* ip, uint16_t port)
{
    if (ip_ver_ == IPVer::V4) {
        local_peer_addr_ = new IPAddr4(ip_addr_.ip_.c_str(), ip_addr_.port_);
    } else {
        local_peer_addr_ = new IPAddr6(ip_addr_.ip_.c_str(), ip_addr_.port_);
    }

    el_logger->info("[UdpPeer::Bind] Bind to address: {}", local_peer_addr_->String());

    if (::bind(fd_, (struct sockaddr*)local_peer_addr_->SockAddr(), local_peer_addr_->Size()) < 0) {
        el_logger->error("[UdpPeer::Bind] Bind to address failed: {}", strerror(errno));
        return false;
    }
    return true;
}

void UdpPeer::OnError(int errcode, const char* errstr)
{
    el_logger->error("[UdpPeer::OnError] error code: {}, error string: {}", errcode, errstr);
    if (on_error_cb_) on_error_cb_(this, errcode, errstr);

    if (errcode == EADDRINUSE || errcode == EADDRNOTAVAIL) {
        exit(1);
    }
}

size_t UdpPeer::ReceivePacket()
{
    char recvbuf[UDP_MAX_BYTES];
    return ReceivePacket(recvbuf, sizeof(recvbuf));
}

size_t UdpPeer::SendPacket(const char* data, size_t size)
{
    return SendPacket(PeerRemoteAddr(), data, size);
}

size_t UdpPeer::SendPacket(const IPAddr* peer_addr, const char* data, size_t size)
{
    el_logger->debug("[UdpPeer::SendPacket] sendto: {}, size: {}", peer_addr->String(), size);
    int tx_size = sendto(fd_, data, size, 0, peer_addr->SockAddr(), peer_addr->Size());
    if (tx_size < 0) {
        el_logger->error("[UdpPeer::SendPacket] sendto failed: {}", strerror(errno));
    }
    return tx_size;
}

size_t UdpPeer::ReceivePacket(char* recvbuf, size_t recvbuf_size)
{
    IPAddr* remote_peer_addr;
    if (ip_ver_ == IPVer::V4) {
        remote_peer_addr = new IPAddr4(ip_addr_.ip_.c_str(), ip_addr_.port_);
    } else {
        remote_peer_addr = new IPAddr6(ip_addr_.ip_.c_str(), ip_addr_.port_);
    }
    socklen_t sock_addr_size = remote_peer_addr->Size();

    size_t rx_bytes = recvfrom(fd_, recvbuf, recvbuf_size, 0, (struct sockaddr*)remote_peer_addr->SockAddr(), &sock_addr_size);
    el_logger->debug("[UdpPeer::ReceivePacket] client: {}, bytes size: {}", remote_peer_addr->String(), rx_bytes);
    if (rx_bytes > 0 && on_packet_cb_) {
        if (!remote_peer_addr_) {
            // set remote peer addr with currently
            remote_peer_addr_ = remote_peer_addr;
        }
        if (on_packet_cb_) {
            on_packet_cb_(this, remote_peer_addr, recvbuf, rx_bytes);
        }
    }

    return rx_bytes;
}

}  // namespace evt_loop

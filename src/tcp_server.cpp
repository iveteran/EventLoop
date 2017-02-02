#include "eventloop.h"
#include "tcp_server.h"
#include <unistd.h>
#include <errno.h>

namespace evt_loop {

TcpServer::TcpServer(const char *host, uint16_t port, MessageType msg_type, TcpCallbacksPtr tcp_evt_cbs)
    : msg_type_(msg_type), tcp_evt_cbs_(tcp_evt_cbs)
{
    InitAddress(host, port);
    Start();
}

TcpServer::~TcpServer()
{
    Destroy();
}

void TcpServer::Destroy()
{
    conn_map_.clear();
    close(fd_);
    SetFD(-1);
}

void TcpServer::InitAddress(const char* host, uint16_t port)
{
    server_addr_.port_ = port;
    if (host[0] == '\0' || strcmp(host, "localhost") == 0) {
        server_addr_.ip_ = "127.0.0.1";
    } else if (strcmp(host, "any") == 0) {
        server_addr_.ip_ = "0.0.0.0";
    } else {
        server_addr_.ip_ = host;
    }
}

void TcpServer::SetTcpCallbacks(const TcpCallbacksPtr& tcp_evt_cbs)
{
    tcp_evt_cbs_ = tcp_evt_cbs;

    FdTcpConnMap::iterator iter;
    for (iter = conn_map_.begin(); iter != conn_map_.end(); ++iter) {
        iter->second->SetTcpCallbacks(tcp_evt_cbs_);
    }
}

void TcpServer::SetNewClientCallback(const OnNewClientCallback& new_client_cb)
{
    new_client_cb_ = new_client_cb;
}

void TcpServer::SetErrorCallback(const OnServerErrorCallback& error_cb)
{
    error_cb_ = error_cb;
}

TcpConnectionPtr TcpServer::GetConnectionByFD(int fd)
{
    FdTcpConnMap::iterator iter = conn_map_.find(fd);
    return (iter != conn_map_.end() ? iter->second : nullptr);
}

bool TcpServer::Start()
{
    int fd = -1;
    if ((fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
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

    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

    sockaddr_in sock_addr;
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = PF_INET;
    sock_addr.sin_port = htons(server_addr_.port_);
    if (inet_aton(server_addr_.ip_.c_str(), &sock_addr.sin_addr) == 0) {
        OnError(errno, strerror(errno));
        return false;
    }

    if (bind(fd, (sockaddr*)&sock_addr, sizeof(sockaddr_in)) == -1 || listen(fd, 4096) == -1) {
        OnError(errno, strerror(errno));
        return false;
    }
    SetFD(fd);

    return true;
}

void TcpServer::OnEvents(uint32_t events)
{
    if (events & FileEvent::READ) {
        IPAddress peer_addr;
        int fd = AcceptClient(peer_addr);
        if (fd > 0) {
            OnNewClient(fd, peer_addr);
        } else {
            events |= FileEvent::ERROR;
        }
    }

    if (events & FileEvent::ERROR) {
        OnError(errno, strerror(errno));
    }
}

int TcpServer::AcceptClient(IPAddress& peer_addr)
{
    struct sockaddr_in sock_addr;
    uint32_t size = sizeof(sock_addr);

    int fd = accept(fd_, (struct sockaddr*)&sock_addr, &size);
    if (fd < 0) {
        OnError(errno, strerror(errno));
        return -1;
    }
    SocketAddrToIPAddress(sock_addr, peer_addr);

    return fd;
}

void TcpServer::OnNewClient(int fd, const IPAddress& peer_addr)
{
    TcpConnectionPtr conn = CreateClient(fd, server_addr_, peer_addr);
    conn->SetMessageType(msg_type_);
    conn_map_.insert(std::make_pair(fd, conn));
    if (new_client_cb_) new_client_cb_(conn.get());
    printf("[TcpServer::OnNewClient] new connection, fd: %d\n", fd);
}

void TcpServer::OnConnectionClosed(TcpConnection* conn)
{
    printf("[TcpServer::OnConnectionClosed] Erase connection, fd: %d\n", conn->FD());
    FdTcpConnMap::iterator iter = conn_map_.find(conn->FD());
    if (iter != conn_map_.end()) {
        //delete iter->second;
        conn_map_.erase(iter);
    }
}

void TcpServer::OnError(int errcode, const char* errstr)
{
    printf("[TcpServer::OnError] error code: %d, error string: %s\n", errcode, errstr);
    if (error_cb_) error_cb_(this, errcode, errstr);
}

//////////////////////////////////////////////

TcpServer6::TcpServer6(const char *host, uint16_t port, bool ipv6_only, MessageType msg_type, TcpCallbacksPtr tcp_evt_cbs)
    : TcpServer(host, port, msg_type, tcp_evt_cbs),
    ipv6_only_(ipv6_only)
{
    InitAddress(host, port);
    Start();
}

void TcpServer6::InitAddress(const char* host, uint16_t port)
{
    server_addr_.port_ = port;
    if (host[0] == '\0' ||
            strcmp(host, "localhost6") == 0 ||
            strcmp(host, "ip6-localhost") == 0 ||
            strcmp(host, "ipv6-localhost") == 0) {
        server_addr_.ip_ = "::1";
    } else if (strcmp(host, "any") == 0) {
        server_addr_.ip_ = "::";
    } else {
        server_addr_.ip_ = host;
    }
}

bool TcpServer6::Start()
{
    int fd = -1;
    if ((fd = socket(PF_INET6, SOCK_STREAM, 0)) == -1) {
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

    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

    sockaddr_in6 sock_addr;
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin6_family = PF_INET6;
    sock_addr.sin6_port = htons(server_addr_.port_);
    if (inet_pton(AF_INET6, server_addr_.ip_.c_str(), &sock_addr.sin6_addr) == 0) {
        OnError(errno, strerror(errno));
        return false;
    }

    if (ipv6_only_) {
        int on = 1;
        if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) == -1) {
            OnError(errno, strerror(errno));
            return false;
        }
    }

    if (bind(fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) == -1 || listen(fd, 4096) == -1) {
        OnError(errno, strerror(errno));
        return false;
    }
    SetFD(fd);

    return true;
}

int TcpServer6::AcceptClient(IPAddress& peer_addr)
{
    struct sockaddr_in6 sock_addr;
    uint32_t size = sizeof(sock_addr);

    int fd = accept(fd_, (struct sockaddr*)&sock_addr, &size);
    if (fd < 0) {
        OnError(errno, strerror(errno));
        return -1;
    }
    SocketAddrToIPAddress(sock_addr, peer_addr);

    return fd;
}

}  // namespace evt_loop

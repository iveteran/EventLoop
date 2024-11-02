#include "eventloop.h"
#include "tcp_server.h"
#include <unistd.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "logger.h"

namespace evt_loop {

TcpServer::TcpServer(IPVer ip_ver, const char *host, uint16_t port, MessageType msg_type, TcpCallbacksPtr tcp_evt_cbs)
    : IOEvent(IOType::TCP_SERVER), ip_ver_(ip_ver), msg_type_(msg_type), msg_hdr_desc_(nullptr), tcp_evt_cbs_(tcp_evt_cbs)
{
    if (ip_ver_ == IPVer::V4) {
        InitAddress(host, port);
    } else {
        InitV6Address(host, port);
    }
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
void TcpServer::InitV6Address(const char* host, uint16_t port)
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

void TcpServer::EnableHeartbeat(uint32_t idle_interval, uint32_t ping_interval, uint32_t ping_total)
{
    hb_tmp_params_ = std::make_shared<HeartbeatParams>(idle_interval, ping_interval, ping_total);

    FdTcpConnMap::iterator iter;
    for (iter = conn_map_.begin(); iter != conn_map_.end(); ++iter) {
        iter->second->EnableHeartbeat(idle_interval, ping_interval, ping_total);
    }
}

void TcpServer::EnableIdleTimeout(uint32_t seconds, const OnIdleTimeoutCallback& cb)
{
    idle_timeout_params_ = std::make_shared<IdleTimeoutParams>(std::make_tuple(seconds, cb));

    FdTcpConnMap::iterator iter;
    for (iter = conn_map_.begin(); iter != conn_map_.end(); ++iter) {
        iter->second->EnableIdleTimeout(seconds, cb);
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

TcpConnectionPtr TcpServer::GetConnectionByFD(int fd)
{
    FdTcpConnMap::iterator iter = conn_map_.find(fd);
    return (iter != conn_map_.end() ? iter->second : nullptr);
}

bool TcpServer::Start()
{
    int fd = -1;
    int domain = ip_ver_ == IPVer::V4 ? AF_INET : AF_INET6;

    if ((fd = socket(domain, SOCK_STREAM, 0)) == -1) {
        OnError(errno, strerror(errno));
        return false;
    }

    int reuseaddr = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) < 0)
    {
        OnError(errno, strerror(errno));
        close(fd);
        return false;
    }

    int qlen = 5;
    if (setsockopt(fd, IPPROTO_TCP, TCP_FASTOPEN, &qlen, sizeof(qlen)) < 0)
    {
        el_logger->warn("[TcpServer::Start] (setsockopt) Ignore error of enabling TFO: {}(errno: {})", strerror(errno), errno);
    }

    struct sockaddr_in sock_addr = {};
    struct sockaddr_in6 sock_addr6 = {};

    if (ip_ver_ == IPVer::V4) {
        sock_addr.sin_family = domain;
        sock_addr.sin_port = htons(server_addr_.port_);
        if (inet_pton(domain, server_addr_.ip_.c_str(), &sock_addr.sin_addr) == 0) {
            OnError(errno, strerror(errno));
            return false;
        }
        if (bind(fd, (struct sockaddr*)&sock_addr, sizeof(sockaddr_in)) < 0) {
            OnError(errno, strerror(errno));
            return false;
        }
    } else {
        sock_addr6.sin6_family = domain;
        sock_addr6.sin6_port = htons(server_addr_.port_);
        if (inet_pton(domain, server_addr_.ip_.c_str(), &sock_addr6.sin6_addr) == 0) {
            OnError(errno, strerror(errno));
            return false;
        }
        if (ip_ver_ == IPVer::V6_ONLY) {
            int on = 1;
            if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) < 0) {
                OnError(errno, strerror(errno));
                return false;
            }
        }
        if (bind(fd, (struct sockaddr*)&sock_addr6, sizeof(sockaddr_in6)) < 0) {
            OnError(errno, strerror(errno));
            return false;
        }
    }

    if (listen(fd, 4096) < 0) {
        OnError(errno, strerror(errno));
        return false;
    }

    el_logger->info("[TcpServer::Start] Listening: {}:{}", server_addr_.ip_, server_addr_.port_);

    SetFD(fd);

    return true;
}

void TcpServer::OnEvents(uint32_t events, void* ctx)
{
    if (events & FileEvent::READ) {
        IPAddress peer_addr;
        int client_fd = -1;
        if (ip_ver_ == IPVer::V4) {
            client_fd = AcceptClient(peer_addr);
        } else {
            client_fd = AcceptClient6(peer_addr);
        }
        if (client_fd > 0) {
            OnNewClient(client_fd, peer_addr);
        } else {
            events |= FileEvent::ERROR;
        }
    } else if (events & FileEvent::CREATE) {
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

int TcpServer::AcceptClient6(IPAddress& peer_addr)
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

void TcpServer::OnNewClient(int fd, const IPAddress& peer_addr)
{
    el_logger->info("[TcpServer::OnNewClient] new connection, fd: {}, msg_type: {}", fd, msg_type_);
    TcpConnectionPtr conn = CreateClient(fd, server_addr_, peer_addr, peer_addr);
    conn->SetMessageType(msg_type_, msg_hdr_desc_);
    if (hb_tmp_params_) {
        conn->EnableHeartbeat(hb_tmp_params_->idle_interval, hb_tmp_params_->ping_interval, hb_tmp_params_->ping_total);
    }
    if (idle_timeout_params_) {
        conn->EnableIdleTimeout(std::get<0>(*idle_timeout_params_), std::get<1>(*idle_timeout_params_));
    }
    conn_map_.insert(std::make_pair(fd, conn));
    if (new_client_cb_) new_client_cb_(conn.get());
}

TcpConnectionPtr
TcpServer::CreateClient(int fd, const IPAddress& local_addr,
        const IPAddress& peer_addr, const IPAddress& peer_real_addr)
{
    return std::make_shared<TcpConnection>(fd, server_addr_, peer_addr, peer_real_addr,
            std::bind(&TcpServer::OnConnectionClosed, this, std::placeholders::_1), tcp_evt_cbs_);
}

void TcpServer::OnConnectionClosed(TcpConnection* conn)
{
    el_logger->info("[TcpServer::OnConnectionClosed] Erase connection, fd: {}", conn->FD());
    conn_map_.erase(conn->FD());
}

void TcpServer::OnError(int errcode, const char* errstr)
{
    el_logger->error("[TcpServer::OnError] error code: {}, error string: {}", errcode, errstr);
    if (error_cb_) error_cb_(this, errcode, errstr);

    if (errcode == EADDRINUSE || errcode == EADDRNOTAVAIL) {
        exit(1);
    }
}

}  // namespace evt_loop

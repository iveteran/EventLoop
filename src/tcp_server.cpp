#include "eventloop.h"
#include "tcp_server.h"
#include <unistd.h>

namespace evt_loop {

TcpServer::TcpServer(const char *host, uint16_t port, MessageType msg_type, TcpCallbacksPtr tcp_evt_cbs)
    : msg_type_(msg_type), tcp_evt_cbs_(tcp_evt_cbs)
{
    server_addr_.port_ = port;
    if (host[0] == '\0' || strcmp(host, "localhost") == 0) {
        server_addr_.ip_ = "127.0.0.1";
    } else if (strcmp(host, "any") == 0) {
        server_addr_.ip_ = "0.0.0.0";
    } else {
        server_addr_.ip_ = host;
    }

    Start();
}

TcpServer::~TcpServer()
{
    Destory();
}

void TcpServer::Destory()
{
    conn_map_.clear();
    close(fd_);
    SetFD(-1);
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
    memset(&sock_addr, 0, sizeof(sockaddr_in));
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
    if (events & IOEvent::READ) {
        struct sockaddr_in sock_addr;
        uint32_t size = sizeof(sock_addr);

        int fd = accept(fd_, (struct sockaddr*)&sock_addr, &size);
        if (fd < 0) {
            OnError(errno, strerror(errno));
            return;
        }
        IPAddress peer_addr;
        SocketAddrToIPAddress(sock_addr, peer_addr);
        OnNewClient(fd, peer_addr);
    }

    if (events & IOEvent::ERROR) {
        OnError(errno, strerror(errno));
    }
}

void TcpServer::OnNewClient(int fd, const IPAddress& peer_addr)
{
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(fd, server_addr_, peer_addr,
          std::bind(&TcpServer::OnConnectionClosed, this, std::placeholders::_1), tcp_evt_cbs_);
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

}  // namespace evt_loop

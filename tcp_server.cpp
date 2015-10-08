#include <map>
#include "tcp_server.h"

using std::map;

namespace richinfo {

TcpServer::TcpServer(const char *host, uint16_t port, ITcpEventHandler* tcp_evt_handler)
    : tcp_evt_handler_(tcp_evt_handler)
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
    map<int, TcpConnection*>::iterator iter;
    for (iter = conn_map_.begin(); iter != conn_map_.end(); ++iter) {
        delete iter->second;
    }
    conn_map_.clear();
    EV_Singleton->DeleteEvent(this);
    close(fd_);
}

void TcpServer::SetTcpEventHandler(ITcpEventHandler* evt_hdlr)
{
    tcp_evt_handler_ = evt_hdlr;

    map<int, TcpConnection*>::iterator iter;
    for (iter = conn_map_.begin(); iter != conn_map_.end(); ++iter) {
        iter->second->SetTcpEventHandler(tcp_evt_handler_);
    }
}

TcpConnection* TcpServer::GetConnectionByFD(int fd)
{
    map<int, TcpConnection*>::iterator iter = conn_map_.find(fd);
    return (iter != conn_map_.end() ? iter->second : NULL);
}

bool TcpServer::Start()
{
    if ((fd_ = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        OnError(errno, strerror(errno));
        return false;
    }
    int reuseaddr = 1;
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1)
    {
        OnError(errno, strerror(errno));
        close(fd_);
        return false;
    }

    int on = 1;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

    sockaddr_in sock_addr;
    memset(&sock_addr, 0, sizeof(sockaddr_in));
    sock_addr.sin_family = PF_INET;
    sock_addr.sin_port = htons(server_addr_.port_);
    if (inet_aton(server_addr_.ip_.c_str(), &sock_addr.sin_addr) == 0) {
        OnError(errno, strerror(errno));
        return false;
    }

    if (bind(fd_, (sockaddr*)&sock_addr, sizeof(sockaddr_in)) == -1 || listen(fd_, 10) == -1) {
        OnError(errno, strerror(errno));
        return false;
    }

    SetFD(fd_);
    EV_Singleton->AddEvent(this);

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
    TcpConnection *conn = new TcpConnection(fd, server_addr_, peer_addr, tcp_evt_handler_, this);
    conn_map_.insert(std::make_pair(fd, conn));
    if (tcp_evt_handler_) tcp_evt_handler_->OnNewConnection(conn);
    printf("[TcpServer::OnNewClient] new client, fd: %d\n", fd);
}

void TcpServer::OnConnectionClosed(TcpConnection* conn)
{
    map<int, TcpConnection*>::iterator iter = conn_map_.find(conn->FD());
    if (iter != conn_map_.end()) {
        delete iter->second;
        conn_map_.erase(iter);
    }
}

void TcpServer::OnError(int errcode, const char* errstr)
{
    printf("[TcpServer::OnError] error code: %d, error string: %s\n", errcode, errstr);
    if (errcode >= ERR_CODE_SERVERITY) {
        // restart acceptor
        Destory();
        Start();
    }
    if (tcp_evt_handler_) tcp_evt_handler_->OnError(errcode, errstr);
}

}  // namespace richinfo

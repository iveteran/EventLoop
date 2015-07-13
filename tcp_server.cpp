#include <map>
#include "tcp_server.h"

using std::map;

namespace richinfo {

TcpServer::TcpServer(const char *host, uint16_t port)
{
    server_addr_.port_ = port;
    if (host[0] == '\0' || strcmp(host, "localhost") == 0) {
        server_addr_.ip_ = "127.0.0.1";
    } else if (strcmp(host, "any") == 0) {
        server_addr_.ip_ = "0.0.0.0";
    } else {
        server_addr_.ip_ = host;
    }

    Listen();
}

void TcpServer::SetOnMessageCb(const OnMessageCallback& on_msg_cb)
{
    on_msg_cb_ = on_msg_cb;
}

bool TcpServer::Listen()
{
    if ((fd_ = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        return false;
    }

    int on = 1;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

    sockaddr_in sock_addr;
    memset(&sock_addr, 0, sizeof(sockaddr_in));
    sock_addr.sin_family = PF_INET;
    sock_addr.sin_port = htons(server_addr_.port_);
    if (inet_aton(server_addr_.ip_.c_str(), &sock_addr.sin_addr) == 0)
        return false;

    if (bind(fd_, (sockaddr*)&sock_addr, sizeof(sockaddr_in)) == -1 || listen(fd_, 10) == -1) {
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
        IPAddress peer_addr;
        SocketAddrToIPAddress(sock_addr, peer_addr);
        OnNewClient(fd, peer_addr);
    }

    if (events & IOEvent::ERROR) {
        close(fd_);
    }
}

void TcpServer::OnNewClient(int fd, const IPAddress& peer_addr)
{
    TcpConnection *conn = new TcpConnection(fd, server_addr_, peer_addr, this);
    conn->SetOnMessageCb(on_msg_cb_);
    printf("[TcpServer::OnNewClient] new client, fd: %d\n", fd);
    conn_map_.insert(std::make_pair(fd, conn));
}

void TcpServer::OnConnectionClosed(TcpConnection* conn)
{
    EV_Singleton->DeleteEvent(conn);
    int fd = conn->FD();
    auto iter = conn_map_.find(fd);
    if (iter != conn_map_.end()) {
        delete iter->second;
        conn_map_.erase(iter);
    }
}

void TcpServer::OnError(const char* errstr)
{
    printf("[TcpServer::OnError] error string: %s\n", errstr);
}

}  // namespace richinfo

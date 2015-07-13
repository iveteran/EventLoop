/*
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
*/

#include "eventloop.h"
#include "tcp_connection.h"

namespace richinfo {

void SocketAddrToIPAddress(const struct sockaddr_in& sock_addr, IPAddress& ip_addr)
{
  char buffer[INET_ADDRSTRLEN] = {0};
  inet_ntop(sock_addr.sin_family, (void*)&sock_addr.sin_addr, buffer, sizeof(buffer));
  ip_addr.ip_.assign(buffer);
  ip_addr.port_ = sock_addr.sin_port;
}

TcpConnection::TcpConnection(int fd, const IPAddress& local_addr, const IPAddress& peer_addr, TcpCreator* creator)
{
    SetFD(fd);
    EV_Singleton->AddEvent(this);
    local_addr_ = local_addr;
    peer_addr_ = peer_addr;
    creator_ = creator;
    printf("[TcpConnection::TcpConnection] local_addr: %s, peer_addr: %s\n", local_addr_.ToString().c_str(), peer_addr_.ToString().c_str());
}

void TcpConnection::SetOnMessageCb(const OnMessageCallback& on_msg_cb)
{
    printf("[TcpConnection::SetOnMessageCb] %s\n", on_msg_cb.ToString().c_str());
    on_msg_cb_ = on_msg_cb;
}

void TcpConnection::OnReceived(const string& buffer)
{
    //printf("[TcpConnection::OnReceived] received string: %s\n", buffer.c_str());
    //on_msg_cb_.Invoke(std::make_tuple(this, &buffer));
    on_msg_cb_.Invoke(this, &buffer);
}

void TcpConnection::OnClosed()
{
    printf("[TcpConnection::OnClosed] client leave, fd: %d\n", fd_);
    close(fd_);
    if (creator_) {
        creator_->OnConnectionClosed(this);
    } else {
        EV_Singleton->DeleteEvent(this);
    }
}

void TcpConnection::OnError(const char* errstr)
{
    printf("[TcpConnection::OnError] error string: %s\n", errstr);
    OnClosed();
}

}  // namespace richinfo

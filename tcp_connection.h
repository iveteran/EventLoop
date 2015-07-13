#ifndef _TCP_CONNECTION_H
#define _TCP_CONNECTION_H

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

#include "eventloop.h"
#include "tcp_callbacks.h"

namespace richinfo {

struct IPAddress {
  string ip_;
  uint16_t port_;
  IPAddress(string ip = "", uint16_t port = 0)
      : ip_(ip), port_(port)
  {}
  string ToString() const {
      char buffer[256] = {0};
      snprintf(buffer, sizeof(buffer), "{ ip: %s, port: %d }", ip_.c_str(), port_);
      return buffer;
  }
};

void SocketAddrToIPAddress(const struct sockaddr_in sock_addr, IPAddress& ip_addr) {
  char buffer[INET_ADDRSTRLEN] = {0};
  inet_ntop(sock_addr.sin_family, (void*)&sock_addr.sin_addr, buffer, sizeof(buffer));
  ip_addr.ip_.assign(buffer);
  ip_addr.port_ = sock_addr.sin_port;
}

class TcpConnection;

class TcpCreator: public IOEvent {
  public:
    TcpCreator() : IOEvent(IOEvent::READ | IOEvent::ERROR) { }
    virtual void OnConnectionClosed(TcpConnection* conn) = 0;
};

class TcpConnection : public BufferIOEvent {
  public:
    TcpConnection(int fd, const IPAddress& local_addr, const IPAddress& peer_addr, TcpCreator* creator = NULL) {
        SetFD(fd);
        EV_Singleton->AddEvent(this);
        local_addr_ = local_addr;
        peer_addr_ = peer_addr;
        creator_ = creator;
        printf("[TcpConnection::TcpConnection] local_addr: %s, peer_addr: %s\n",
                local_addr_.ToString().c_str(), peer_addr_.ToString().c_str());
    }

    void SetOnMessageCb(const OnMessageCallback& on_msg_cb) {
        printf("[TcpConnection::SetOnMessageCb] %s\n", on_msg_cb.ToString().c_str());
        on_msg_cb_ = on_msg_cb;
    }

  protected:
    void OnReceived(const string& buffer) {
        //printf("[TcpConnection::OnReceived] received string: %s\n", buffer.c_str());
        //on_msg_cb_.Invoke(std::make_tuple(this, &buffer));
        on_msg_cb_.Invoke(this, &buffer);
    }

  private:
    void OnClosed() {
      printf("[TcpConnection::OnClosed] client leave, fd: %d\n", fd_);
      close(fd_);
      if (creator_) {
          creator_->OnConnectionClosed(this);
      } else {
          EV_Singleton->DeleteEvent(this);
      }
    }

    void OnError(char* errstr) {
        printf("[TcpConnection::OnError] error string: %s\n", errstr);
        OnClosed();
    }

  private:
    IPAddress local_addr_;
    IPAddress peer_addr_;
    TcpCreator* creator_;

    OnMessageCallback on_msg_cb_;
};

}  // namespace richinfo

#endif  // _TCP_CONNECTION_H

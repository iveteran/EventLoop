#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

#include <map>
#include "tcp_connection.h"

using std::map;

namespace richinfo {

class TcpServer: public TcpCreator {
  public:
    TcpServer(const char *host, uint16_t port) {
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

    void SetOnMessageCb(const OnMessageCallback& on_msg_cb) {
        on_msg_cb_ = on_msg_cb;
    }

 private:
    bool Listen() {

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

      return true; // TODO
    }

  void OnEvents(uint32_t events) {
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

  void OnNewClient(int fd, IPAddress& peer_addr) {
    TcpConnection *conn = new TcpConnection(fd, server_addr_, peer_addr, this);
    conn->SetOnMessageCb(on_msg_cb_);
    printf("[TcpServer::OnNewClient] new client, fd: %d\n", fd);
    conn_map_.insert(std::make_pair(fd, conn));
  }

  void OnConnectionClosed(TcpConnection* conn) {
      EV_Singleton->DeleteEvent(conn);
      int fd = conn->FD();
      auto iter = conn_map_.find(fd);
      if (iter != conn_map_.end()) {
          delete iter->second;
          conn_map_.erase(iter);
      }
  }

  void OnError(char* errstr) {
      printf("[TcpServer::OnError] error string: %s\n", errstr);
  }

 private:
  IPAddress server_addr_;
  map<int/*fd*/, TcpConnection*> conn_map_;
  OnMessageCallback on_msg_cb_;
};

}  // namespace richinfo

#endif  // _TCP_SERVER_H

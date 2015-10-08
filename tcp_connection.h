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
#include "error_code.h"

namespace evt_loop {

struct IPAddress
{
  string ip_;
  uint16_t port_;

  IPAddress(string ip = "", uint16_t port = 0) : ip_(ip), port_(port) {}
  string ToString() const
  {
      char buffer[256] = {0};
      snprintf(buffer, sizeof(buffer), "{ ip: %s, port: %d }", ip_.c_str(), port_);
      return buffer;
  }
};

void SocketAddrToIPAddress(const struct sockaddr_in& sock_addr, IPAddress& ip_addr);

class TcpConnection;

class TcpCreator: public IOEvent
{
  protected:
    TcpCreator() : IOEvent(IOEvent::READ | IOEvent::ERROR) {}
  public:
    virtual void OnConnectionClosed(TcpConnection* conn) = 0;
};

class TcpConnection : public BufferIOEvent
{
  public:
    TcpConnection(int fd, const IPAddress& local_addr, const IPAddress& peer_addr,
            ITcpEventHandler* tcp_evt_handler = NULL, TcpCreator* creator = NULL);
    ~TcpConnection();

    void Disconnect();

    void SetTcpEventHandler(ITcpEventHandler* evt_handler);

    void SetReady(int fd);
    bool IsReady() const;

    const IPAddress& GetLocalAddr() const;
    const IPAddress& GetPeerAddr() const;

  protected:
    void OnReceived(const string& buffer);
    void OnSent(const string& buffer);
    void OnClosed();
    void OnError(int errcode, const char* errstr);

  private:
    bool            ready_;
    IPAddress       local_addr_;
    IPAddress       peer_addr_;

    ITcpEventHandler*   tcp_evt_handler_;
    TcpCreator*         creator_;
};

}  // namespace evt_loop

#endif  // _TCP_CONNECTION_H

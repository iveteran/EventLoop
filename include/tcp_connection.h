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

#include <map>

#include "eventloop.h"
#include "tcp_callbacks.h"
#include "error_code.h"
#include "utils.h"

namespace evt_loop {

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
            TcpCallbacksPtr tcp_evt_cbs = nullptr, TcpCreator* creator = nullptr);
    ~TcpConnection();

    uint32_t ID() const { return id_; }
    void SetID(uint32_t id) { id_ = id; }

    void Disconnect();

    void SetTcpCallbacks(const TcpCallbacksPtr& tcp_evt_cbs);

    void SetReady(int fd);
    bool IsReady() const;

    const IPAddress& GetLocalAddr() const;
    const IPAddress& GetPeerAddr() const;

  protected:
    void OnReceived(const Message* buffer);
    void OnSent(const Message* buffer);
    void OnClosed();
    void OnError(int errcode, const char* errstr);

  private:
    uint32_t        id_;
    bool            ready_;
    IPAddress       local_addr_;
    IPAddress       peer_addr_;

    TcpCallbacksPtr tcp_evt_cbs_;
    TcpCreator*     creator_;
};

typedef std::shared_ptr<TcpConnection>          TcpConnectionPtr;
typedef std::map<int/*fd*/, TcpConnectionPtr>   FdTcpConnMap;

}  // namespace evt_loop

#endif  // _TCP_CONNECTION_H

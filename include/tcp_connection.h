#ifndef _TCP_CONNECTION_H
#define _TCP_CONNECTION_H

#include <map>
#include <memory>

#include "fd_handler.h"
#include "tcp_callbacks.h"
#include "utils.h"

using std::map;
using std::shared_ptr;

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

typedef shared_ptr<TcpConnection>          TcpConnectionPtr;
typedef map<int/*fd*/, TcpConnectionPtr>   FdTcpConnMap;

}  // namespace evt_loop

#endif  // _TCP_CONNECTION_H

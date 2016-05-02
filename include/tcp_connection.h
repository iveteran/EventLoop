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

class TcpConnection : public BufferIOEvent
{
  public:
    TcpConnection(int fd, const IPAddress& local_addr, const IPAddress& peer_addr,
            const OnClosedCallback& close_cb, TcpCallbacksPtr tcp_evt_cbs = nullptr);
    ~TcpConnection();

    uint32_t ID() const { return id_; }
    void SetID(uint32_t id) { id_ = id; }

    void Disconnect();

    void SetTcpCallbacks(const TcpCallbacksPtr& tcp_evt_cbs);

    const IPAddress& GetLocalAddr() const;
    const IPAddress& GetPeerAddr() const;

  protected:
    void OnReceived(const Message* buffer);
    void OnSent(const Message* buffer);
    void OnClosed();
    void OnError(int errcode, const char* errstr);

  private:
    uint32_t        id_;
    IPAddress       local_addr_;
    IPAddress       peer_addr_;

    OnClosedCallback  creator_notification_cb_;
    TcpCallbacksPtr   tcp_evt_cbs_;
};

typedef shared_ptr<TcpConnection>          TcpConnectionPtr;
typedef map<int/*fd*/, TcpConnectionPtr>   FdTcpConnMap;

}  // namespace evt_loop

#endif  // _TCP_CONNECTION_H

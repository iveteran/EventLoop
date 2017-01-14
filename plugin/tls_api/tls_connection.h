#pragma once
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "tcp_connection.h"

namespace evt_loop {

struct TLSConnection : public TcpConnection
{
  public:
   TLSConnection(int fd, const IPAddress& local_addr, const IPAddress& peer_addr,
            const OnClosedCallback& close_cb, TcpCallbacksPtr tcp_evt_cbs = nullptr);
   ~TLSConnection();
   static void setSSLCertKey(const char* cert, const char* key, const char* ca_cert = NULL);

  protected:
  virtual void OnHandshake();
  virtual int OnRead(const void* buf, size_t bytes);
  virtual int OnWrite(const void* buf, size_t bytes);

  private:
  SSL *ssl_;
};

};

#include "cdb_client.h"

namespace cdb_api
{

bool CDBClient::Init(const char* host, uint16_t port, const CDBCallbacksPtr& cdb_cbs, bool auto_reconnect)
{
  auto_reconnect_ = auto_reconnect,
  server_addr_.port_ = port;
  cdb_cbs_ = cdb_cbs;

  if (host[0] == '\0' || strcmp(host, "localhost") == 0) {
    server_addr_.ip_ = "127.0.0.1";
  } else if (!strcmp(host, "any")) {
    server_addr_.ip_ = "0.0.0.0";
  } else {
    server_addr_.ip_ = host;
  }

  if (auto_reconnect_) {
    reconnect_timer_.SetInterval(TimeVal(1, 0));
  }

  return Connect();
}

CDBClient::~CDBClient()
{
  Disconnect();
}

bool CDBClient::Connect()
{
  bool success = Connect_();
  if (!success && auto_reconnect_)
    Reconnect();
  return success;
}

void CDBClient::Reconnect()
{
  //Disconnect();
  if (!reconnect_timer_.IsRunning())
    reconnect_timer_.Start();
}

void CDBClient::OnReconnectTimer(TimerEvent* timer)
{
  //printf("[CDBClient::OnReconnectTimer begin] is ready: %d\n", IsReady());
  if (!IsReady()) {  // if the connection is not created, then reconnect
    bool success = Connect_(RECONNECT);
    if (success)
      timer->Stop();
    else
      printf("[CDBClient::OnReconnectTimer] Reconnect failed, retry %u seconds later...\n", timer->GetInterval().Seconds());
  } else {
    timer->Stop();
  }
}

}  // namespace cdb_api

#ifndef _UTILS_H
#define _UTILS_H

#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <vector>

using std::string;

namespace evt_loop {

class TimeVal {
 public:
  TimeVal(uint32_t sec = 0, uint32_t usec = 0);
  TimeVal(const timeval& time);
  TimeVal& SetNow();

  timeval Value() const;
  uint32_t Seconds()  const;
  uint32_t USeconds() const;

  bool operator ==(const TimeVal& other) const;
   bool operator !=(const TimeVal& other) const;
  bool operator <(const TimeVal& other) const;
  bool operator >(const TimeVal& other) const;
  TimeVal operator-(const TimeVal& other) const;
  TimeVal operator+(const TimeVal& other) const;

  static int32_t MsDiff(const TimeVal& lv, const TimeVal& rv);
  static TimeVal Now();

 private:
  timeval tv_;
};

struct IPAddress
{
  string ip_;
  uint16_t port_;

  IPAddress(string ip = "", uint16_t port = 0) : ip_(ip), port_(port) {}
  string ToString() const
  {
      char buffer[64] = {0};
      snprintf(buffer, sizeof(buffer), "%s:%d", ip_.c_str(), port_);
      return buffer;
  }
  string ToJSON() const
  {
      char buffer[64] = {0};
      snprintf(buffer, sizeof(buffer), "{ ip: %s, port: %d }", ip_.c_str(), port_);
      return buffer;
  }
};
typedef std::vector<IPAddress> IPAddressList;

void SocketAddrToIPAddress(const struct sockaddr_in& sock_addr, IPAddress& ip_addr);
void SocketAddrToIPAddress(const struct sockaddr_in6& sock_addr, IPAddress& ip_addr);

}  // namespace evt_loop

#endif  // _UTILS_H

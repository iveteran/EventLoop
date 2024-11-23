#ifndef _IP_ADDR_H
#define _IP_ADDR_H

#include <string>
#include <vector>
#include <arpa/inet.h>

using std::string;

namespace evt_loop {

enum IPVer {
    V4,
    V6,
    V6_ONLY,
};

struct IPAddr
{
    virtual ~IPAddr() {}
    virtual bool SetIP(const char* ip) = 0;
    virtual void SetPort(uint16_t port) = 0;
    virtual const struct sockaddr* SockAddr() const = 0;
    virtual size_t Size() const = 0;
    virtual string IP() const = 0;
    virtual uint16_t Port() const = 0;
    virtual string String() const = 0;
};

class IPAddr4 : public IPAddr
{
    public:
    IPAddr4(const char* ip=nullptr, uint16_t port=0);
    IPAddr4(struct sockaddr_in& sock_addr);
    void Assign(const char* ip, uint16_t port);
    bool SetIP(const char* ip);
    void SetPort(uint16_t port);
    const struct sockaddr* SockAddr() const;
    size_t Size() const;
    string IP() const;
    uint16_t Port() const;
    string String() const;

    private:
    struct sockaddr_in sock_addr_;
};

class IPAddr6 : public IPAddr
{
    public:
    IPAddr6(const char* ip=nullptr, uint16_t port=0);
    IPAddr6(struct sockaddr_in6& sock_addr);
    void Assign(const char* ip, uint16_t port);
    bool SetIP(const char* ip);
    void SetPort(uint16_t port);
    const struct sockaddr* SockAddr() const;
    size_t Size() const;
    string IP() const;
    uint16_t Port() const;
    string String() const;

    private:
    struct sockaddr_in6 sock_addr_;
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

}  // ns evt_loop
#endif  // _IP_ADDR_H

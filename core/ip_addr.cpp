#include "ip_addr.h"
#include "logger.h"

namespace evt_loop {

IPAddr4::IPAddr4(struct sockaddr_in& sock_addr) {
    sock_addr_ = sock_addr;
}

IPAddr4::IPAddr4(const char* ip, uint16_t port) {
    Assign(ip, port);
}

void IPAddr4::Assign(const char* ip, uint16_t port) {
    memset(&sock_addr_, 0, sizeof(sock_addr_));
    sock_addr_.sin_family = PF_INET;
    SetPort(port);
    SetIP(ip);
}

bool IPAddr4::SetIP(const char* ip) {
    if (! ip) return true;
    if (inet_aton(ip, &sock_addr_.sin_addr) == 0) {
        el_logger->error("[IPAddr4::SetIP] IP: {}, failed: {}", ip, strerror(errno));
        return false;
    }
    return true;
}

void IPAddr4::SetPort(uint16_t port) {
    if (! port) return;
    sock_addr_.sin_port = htons(port);
}

const struct sockaddr* IPAddr4::SockAddr() const {
    return (struct sockaddr*)&sock_addr_;
}

size_t IPAddr4::Size() const {
    return sizeof(sock_addr_);
}

string IPAddr4::IP() const {
    char buf[64];
    inet_ntop(PF_INET, &sock_addr_.sin_addr, buf, sizeof(buf));
    return buf;
}

uint16_t IPAddr4::Port() const {
    return ntohs(sock_addr_.sin_port);
}

string IPAddr4::String() const {
    char buf[64];
    snprintf(buf, sizeof(buf), "%s:%d", IP().c_str(), Port());
    return buf;
}

IPAddr* IPAddr4::Clone() const {
    IPAddr4* clone = new IPAddr4();
    *clone = *this;
    return clone;
}

IPAddr6::IPAddr6(struct sockaddr_in6& sock_addr) {
    sock_addr_ = sock_addr;
}
IPAddr6::IPAddr6(const char* ip, uint16_t port) {
    Assign(ip, port);
}
void IPAddr6::Assign(const char* ip, uint16_t port) {
    memset(&sock_addr_, 0, sizeof(sock_addr_));
    sock_addr_.sin6_family = PF_INET6;
    SetPort(port);
    SetIP(ip);
}
bool IPAddr6::SetIP(const char* ip) {
    if (! ip) return true;
    if (inet_pton(PF_INET6, ip, &sock_addr_.sin6_addr) == 0) {
        el_logger->error("[IPAddr6::SetIP] IP: {}, failed: {}", ip, strerror(errno));
        return false;
    }
    return true;
}
void IPAddr6::SetPort(uint16_t port) {
    if (! port) return;
    sock_addr_.sin6_port = htons(port);
}
const struct sockaddr* IPAddr6::SockAddr() const {
    return (struct sockaddr*)&sock_addr_;
}
size_t IPAddr6::Size() const {
    return sizeof(sock_addr_);
}
string IPAddr6::IP() const {
    char buf[64];
    inet_ntop(PF_INET6, &sock_addr_.sin6_addr, buf, sizeof(buf));
    return buf;
}
uint16_t IPAddr6::Port() const {
    return ntohs(sock_addr_.sin6_port);
}
string IPAddr6::String() const {
    char buf[64];
    snprintf(buf, sizeof(buf), "%s:%d", IP().c_str(), Port());
    return buf;
}

IPAddr* IPAddr6::Clone() const {
    IPAddr6* clone = new IPAddr6();
    *clone = *this;
    return clone;
}

void SocketAddrToIPAddress(const struct sockaddr_in& sock_addr, IPAddress& ip_addr)
{
  char buffer[INET_ADDRSTRLEN] = {0};
  inet_ntop(sock_addr.sin_family, (void*)&sock_addr.sin_addr, buffer, sizeof(buffer));
  ip_addr.ip_.assign(buffer);
  ip_addr.port_ = sock_addr.sin_port;
}

void SocketAddrToIPAddress(const struct sockaddr_in6& sock_addr, IPAddress& ip_addr)
{
  char buffer[INET6_ADDRSTRLEN] = {0};
  inet_ntop(sock_addr.sin6_family, (void*)&sock_addr.sin6_addr, buffer, sizeof(buffer));
  ip_addr.ip_.assign(buffer);
  ip_addr.port_ = sock_addr.sin6_port;
}

}  // ns evt_loop

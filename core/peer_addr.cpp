#include "peer_addr.h"
#include "logger.h"

namespace evt_loop {

PeerAddr4::PeerAddr4(const char* ip, uint16_t port) {
    Assign(ip, port);
}

void PeerAddr4::Assign(const char* ip, uint16_t port) {
    memset(&sock_addr_, 0, sizeof(sock_addr_));
    sock_addr_.sin_family = PF_INET;
    SetPort(port);
    SetIP(ip);
}

bool PeerAddr4::SetIP(const char* ip) {
    if (! ip) return true;
    if (inet_aton(ip, &sock_addr_.sin_addr) == 0) {
        el_logger->error("[PeerAddr4::SetIP] failed: {}", strerror(errno));
        return false;
    }
    return true;
}

void PeerAddr4::SetPort(uint16_t port) {
    if (! port) return;
    sock_addr_.sin_port = htons(port);
}

const struct sockaddr* PeerAddr4::SockAddr() const {
    return (struct sockaddr*)&sock_addr_;
}

size_t PeerAddr4::Size() const {
    return sizeof(sock_addr_);
}

string PeerAddr4::IP() const {
    char buf[64];
    inet_ntop(AF_INET, &sock_addr_.sin_addr, buf, sizeof(buf));
    return buf;
}

uint16_t PeerAddr4::Port() const {
    return ntohs(sock_addr_.sin_port);
}

string PeerAddr4::String() const {
    char buf[64];
    snprintf(buf, sizeof(buf), "%s:%d", IP().c_str(), Port());
    return buf;
}

PeerAddr6::PeerAddr6(const char* ip, uint16_t port) {
    Assign(ip, port);
}
void PeerAddr6::Assign(const char* ip, uint16_t port) {
    memset(&sock_addr_, 0, sizeof(sock_addr_));
    sock_addr_.sin6_family = PF_INET6;
    SetPort(port);
    SetIP(ip);
}
bool PeerAddr6::SetIP(const char* ip) {
    if (! ip) return true;
    if (inet_pton(AF_INET6, ip, &sock_addr_.sin6_addr) == 0) {
        el_logger->error("[PeerAddr6::SetIP] failed: {}", strerror(errno));
        return false;
    }
    return true;
}
void PeerAddr6::SetPort(uint16_t port) {
    if (! port) return;
    sock_addr_.sin6_port = htons(port);
}
const struct sockaddr* PeerAddr6::SockAddr() const {
    return (struct sockaddr*)&sock_addr_;
}
size_t PeerAddr6::Size() const {
    return sizeof(sock_addr_);
}
string PeerAddr6::IP() const {
    char buf[64];
    inet_ntop(AF_INET6, &sock_addr_.sin6_addr, buf, sizeof(buf));
    return buf;
}
uint16_t PeerAddr6::Port() const {
    return ntohs(sock_addr_.sin6_port);
}
string PeerAddr6::String() const {
    char buf[64];
    snprintf(buf, sizeof(buf), "%s:%d", IP().c_str(), Port());
    return buf;
}

}  // ns evt_loop

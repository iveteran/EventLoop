#ifndef _PEER_ADDR_H
#define _PEER_ADDR_H

#include <string>
#include <arpa/inet.h>

using std::string;

namespace evt_loop {

enum IPVer {
    V4,
    V6,
    V6_ONLY,
};

struct PeerAddr
{
    virtual bool SetIP(const char* ip) = 0;
    virtual void SetPort(uint16_t port) = 0;
    virtual const struct sockaddr* SockAddr() const = 0;
    virtual size_t Size() const = 0;
    virtual string IP() const = 0;
    virtual uint16_t Port() const = 0;
    virtual string String() const = 0;
};

class PeerAddr4 : public PeerAddr
{
    public:
    PeerAddr4(const char* ip=nullptr, uint16_t port=0);
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

class PeerAddr6 : public PeerAddr
{
    public:
    PeerAddr6(const char* ip=nullptr, uint16_t port=0);
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

}  // ns evt_loop
#endif  // _PEER_ADDR_H

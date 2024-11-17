#ifndef _ENET_CLIENT_H
#define _ENET_CLIENT_H

#include "enet_event_handler.h"

namespace evt_loop {

class ENetClient : public ENetEventHandler {
    public:
    ENetClient();
    bool Connect(uint16_t port, const char* ip);
    void Disconnect();

    private:
    ENetPeer* peer_ = nullptr;
};

}  // ns evt_loop

#endif  // _ENET_CLIENT_H

#ifndef _ENET_SERVER_H
#define _ENET_SERVER_H

#include "enet_event_handler.h"

namespace evt_loop {

class ENetServer : public ENetEventHandler {
    public:
    ENetServer(uint16_t port, const char* ip = nullptr);
    bool init(uint16_t port, const char* ip = nullptr);
};

}  // ns evt_loop

#endif  // _ENET_SERVER_H

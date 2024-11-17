#include <stdio.h>
#include "enet_server.h"
#include "eventloop/logger.h"

#define DFT_MAX_CLIENTS 4095 // XXX: the ENet only supports max value 4095
#define DFT_CHANNELS 8

namespace evt_loop {

ENetServer::ENetServer(uint16_t port, const char* ip) :
    ENetEventHandler(DFT_MAX_CLIENTS, DFT_CHANNELS) {
    bool success = init(port, ip);
    assert(success);
}

bool ENetServer::init(uint16_t port, const char* ip) {
    if (enet_initialize() != 0) {
        el_logger->error("[ENetServer::init] Error initializing ENet.");
        return false;
    }

    ENetAddress address;
    if (ip) {
        enet_address_set_host_ip(&address, ip);
    } else {
        address.host = ENET_HOST_ANY;
    }
    address.port = port;

    return ENetEventHandler::init(&address);
}

}  // ns evt_loop

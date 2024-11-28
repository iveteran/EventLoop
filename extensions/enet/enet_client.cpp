#include <stdio.h>
#include "enet_client.h"
#include "eventloop/logger.h"

#define DFT_MAX_CLIENTS 64 // XXX: the ENet only supports max value 4095
#define DFT_CHANNELS 8

namespace evt_loop {

ENetClient::ENetClient() : ENetEventHandler(DFT_MAX_CLIENTS, DFT_CHANNELS) {
    bool success = init();
    assert(success);
}

bool ENetClient::Connect(IPVer ip_ver, uint16_t port, const char* ip) {
    // XXX: Currently the ENet only supports IP v4
    ENetAddress address;
    enet_address_set_host_ip(&address, ip);
    address.port = port;

    // Connect to the server
    peer_ = enet_host_connect(host_, &address, channel_limit_, 0);
    if (peer_ == NULL) {
        el_logger->error("[ENetClient::connect] No available peers for initiating an ENet connection");
        return false;
    }
    el_logger->info("[ENetClient::connect] connect to: {}:{}", ip, address.port);

    DoENetService();

    return true;
}

void ENetClient::Disconnect() {
    if (peer_) {
        enet_peer_disconnect(peer_, 0);
        DoENetService();  // handle disconnecting process
    }
}

}  // ns evt_loop

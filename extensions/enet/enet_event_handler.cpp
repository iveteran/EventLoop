#include <stdio.h>
#include "enet_event_handler.h"
#include "eventloop/logger.h"

namespace evt_loop {

ENetEventHandler::ENetEventHandler(
        size_t clients_limit,
        size_t channel_limit,
        uint32_t incoming_bandwidth,
        uint32_t outgoing_bandwidth) :
    clients_limit_(clients_limit),
    channel_limit_(channel_limit),
    incoming_bandwidth_(incoming_bandwidth),
    outgoing_bandwidth_(outgoing_bandwidth)
{
}

bool ENetEventHandler::init(ENetAddress* address) {
    if (enet_initialize() != 0) {
        el_logger->error("[ENetEventHandler::init] Error initializing ENet.");
        return false;
    }

    el_logger->info("[ENetEventHandler::init] max clients: {}, channel limit: {}, incoming bandwidth: {}, outgoing bandwidth: {}",
            clients_limit_, channel_limit_, incoming_bandwidth_, outgoing_bandwidth_);

    host_ = enet_host_create(address, clients_limit_, channel_limit_, incoming_bandwidth_, outgoing_bandwidth_);
    if (! host_) {
        el_logger->error("[ENetEventHandler::init] Error creating ENet server host");
        return false;
    }
    if (address) {
        char listen_ip[64];
        enet_address_get_host_ip(address, listen_ip, sizeof(listen_ip));
        el_logger->info("[ENetEventHandler::init] ENet listen on {}:{}", listen_ip, address->port);
    }

    SetFD(host_->socket);

    return true;
}

void ENetEventHandler::destory() {
    enet_host_destroy(host_);
    enet_deinitialize();
}

void ENetEventHandler::OnEvents(uint32_t events, void* ctx) {
    if (events & FileEvent::READ ||
            events & FileEvent::WRITE) {
        DoENetService();
    }

    if (events & FileEvent::ERROR) {
        OnError(errno, strerror(errno));
    }
}

void ENetEventHandler::OnError(int errcode, const char* errmsg) {
    el_logger->error("[ENetEventHandler::OnError] {}", errmsg);
    if (on_error_cb_) {
        on_error_cb_(this, errno, errmsg);
    }
}

void ENetEventHandler::DoENetService() {
    ENetEvent event;
    int timeout = 50; // milliseconds
    while (enet_host_service(host_, &event, timeout) > 0) {
        auto peer = event.peer;
        char peer_ip[64];
        enet_address_get_host_ip(&peer->address, peer_ip, sizeof(peer_ip));
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                el_logger->debug("[ENetEventHandler::DoENetService] Client connected from {}:{}",
                        peer_ip, peer->address.port);
                peer->data = NULL;
                if (on_peer_connected_cb_) {
                    on_peer_connected_cb_(peer);
                }
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                el_logger->debug("[ENetClient::DoENetService] Client: {}, address: {}:{} disconnected.", 
                        peer->data ? (char*)peer->data : "unknown", peer_ip, peer->address.port);
                peer->data = NULL;
                if (on_peer_disconnect_cb_) {
                    on_peer_disconnect_cb_(peer);
                }
                enet_peer_reset(peer);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                {
                    auto packet = event.packet;
                    el_logger->debug("[ENetEventHandler::DoENetService] Received packet of length {} on channel {}",
                            packet->dataLength, (int)event.channelID);

                    if (on_packet_received_cb_) {
                        on_packet_received_cb_(peer, (char*)packet->data, packet->dataLength);
                    }

                    // Clean up the packet
                    enet_packet_destroy(packet);
                }
                break;
            case ENET_EVENT_TYPE_NONE:
                break;
            default:
                break;
        }
    }
}

size_t ENetEventHandler::SendPacket(ENetPeer* peer, const char* data, size_t size) {
    ENetPacket* packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);
    enet_host_flush(host_);
    return size;
}

}  // ns evt_loop

#ifndef _ENET_EVENT_HANDLER_H
#define _ENET_EVENT_HANDLER_H

#include <enet/enet.h>
#include "eventloop/io_event.h"

namespace evt_loop {

class ENetEventHandler;

using ENetOnPeerConnectedCallback = std::function<void (ENetPeer*)>;
using ENetOnPeerDisconnectCallback = std::function<void (ENetPeer*)>;
using ENetOnPacketReceivedCallback = std::function<void (ENetPeer*, const char* data, size_t size)>;
using ENetOnErrorCallback = std::function<void (ENetEventHandler*, int errcode, const char* errmsg)>;

class ENetEventHandler : public IOEvent {
    public:
    static const int DFT_MAX_CLIENTS = 4095; // XXX: the ENet only supports max value 4095

    ENetEventHandler(
            size_t clients_limit = DFT_MAX_CLIENTS,
            size_t channel_limit = 256,
            uint32_t imcoming_bandwidth = 0,
            uint32_t outgoing_bandwidth = 0);
    virtual ~ENetEventHandler() {
        destory();
    }
    bool init(ENetAddress* address = nullptr);
    void destory();

    void SetOnClientConnectedCallback(const ENetOnPeerConnectedCallback& cb) { on_client_connected_cb_ = cb; }
    void SetOnPeerConnectedCallback(const ENetOnPeerConnectedCallback& cb) { on_peer_connected_cb_ = cb; }
    void SetOnPeerDisconnectedCallback(const ENetOnPeerDisconnectCallback& cb) { on_peer_disconnect_cb_ = cb; }
    void SetOnPacketReceivedCallback(const ENetOnPacketReceivedCallback& cb) { on_packet_received_cb_ = cb; }
    void SetOnErrorCallback(const ENetOnErrorCallback& cb) { on_error_cb_ = cb; }

    size_t SendPacket(ENetPeer* peer, const char* data, size_t size);

    protected:
    void OnEvents(uint32_t events, void* ctx) override;
    void OnError(int errcode, const char* errmsg) override;
    void DoENetService();

    protected:
    size_t clients_limit_;
    size_t channel_limit_;
    uint32_t incoming_bandwidth_;
    uint32_t outgoing_bandwidth_;

    ENetHost* host_ = nullptr;

    ENetOnPeerConnectedCallback on_client_connected_cb_;
    ENetOnPeerConnectedCallback on_peer_connected_cb_;
    ENetOnPeerDisconnectCallback on_peer_disconnect_cb_;
    ENetOnPacketReceivedCallback on_packet_received_cb_;
    ENetOnErrorCallback on_error_cb_;
};

}  // ns evt_loop

#endif  // _ENET_EVENT_HANDLER_H

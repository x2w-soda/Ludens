#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/NetworkTransport/NetworkTransportDef.h>

namespace LD {

/// @brief TCP server handle.
struct TCPServer : Handle<struct TCPServerObj>
{
    static TCPServer create(uint16_t port);
    static void destroy(TCPServer server);

    void poll();
    void send(View bytes);
    void set_user(void* user);
    void set_on_recv(TransportByteFn fn);
};

} // namespace LD
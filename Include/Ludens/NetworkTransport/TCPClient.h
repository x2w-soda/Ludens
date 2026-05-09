#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/NetworkTransport/NetworkTransportDef.h>

namespace LD {

/// @brief TCP client handle.
struct TCPClient : Handle<struct TCPClientObj>
{
    static TCPClient create();
    static void destroy(TCPClient client);

    void poll();
    void connect(const char* host, uint16_t port);
    void send(View bytes);
    void set_user(void* user);
    void set_on_recv(TransportByteFn fn);
    void set_on_connect(TransportEndpointFn fn);
};

} // namespace LD
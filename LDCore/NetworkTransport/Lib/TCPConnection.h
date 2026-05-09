#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/NetworkTransport/NetworkTransportDef.h>

// We are in a private module header,
// otherwise this would be very sinful.
#include <asio.hpp>
using asio::ip::tcp;

namespace LD {

struct TCPConnection
{
    tcp::socket socket;
    Vector<byte> recvBuffer;
    TransportByteFn onRecv = nullptr;
    TransportEndpointFn onConnect = nullptr;
    void* user = nullptr;
    bool isConnected = false;

    TCPConnection() = delete;
    TCPConnection(asio::io_context& io)
        : socket(io) {}

    void start_read();
    void async_accept(tcp::acceptor& acceptor);
    void async_connect(const asio::ip::basic_resolver_results<tcp>& endpoints);
    void async_write(View bytes);
    void async_read_some();
};

} // namespace LD
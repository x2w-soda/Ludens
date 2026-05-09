#include <Ludens/DSA/ViewUtil.h>

#include <cstdio> // BUFSIZ

#include "TCPConnection.h"

namespace LD {

void TCPConnection::async_connect(const asio::ip::basic_resolver_results<tcp>& endpoints)
{
    asio::async_connect(socket, endpoints, [this](std::error_code ec, tcp::endpoint endpoint) {
        if (ec)
            return;

        isConnected = true;

        if (onConnect)
        {
            TransportEndpoint ep{};
            ep.host = endpoint.address().to_string().c_str();
            ep.port = endpoint.port();
            onConnect(ep, user);
        }

        start_read();
    });
}

void TCPConnection::start_read()
{
    recvBuffer.resize(BUFSIZ);

    async_read_some();
}

void TCPConnection::async_accept(tcp::acceptor& acceptor)
{
    acceptor.async_accept(socket, [this](std::error_code ec) {
        if (ec)
            return;

        isConnected = true;
        start_read();
    });
}

void TCPConnection::async_write(View bytes)
{
    asio::async_write(
        socket,
        asio::buffer(bytes.data, bytes.size),
        [](std::error_code ec, std::size_t) {
            // TODO:
        });
}

void TCPConnection::async_read_some()
{
    socket.async_read_some(asio::buffer(recvBuffer),
                           [this](std::error_code ec, std::size_t bytes) {
                               if (ec)
                                   return;

                               if (onRecv)
                                   onRecv(View(recvBuffer.data(), bytes), user);

                               start_read();
                           });
}

} // namespace LD
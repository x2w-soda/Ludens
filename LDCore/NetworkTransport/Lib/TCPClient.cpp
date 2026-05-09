#include <Ludens/Memory/Memory.h>
#include <Ludens/NetworkTransport/TCPClient.h>
#include <Ludens/Profiler/Profiler.h>

#include "TCPConnection.h"

#include <iostream>

namespace LD {

struct TCPClientObj
{
    asio::io_context io = {};
    tcp::resolver resolver;
    TCPConnection conn;

    TCPClientObj();
};

TCPClientObj::TCPClientObj()
    : resolver(io), conn(io)
{
}

TCPClient TCPClient::create()
{
    auto* obj = heap_new<TCPClientObj>(MEMORY_USAGE_NETWORK);

    return TCPClient(obj);
}

void TCPClient::destroy(TCPClient client)
{
    auto* obj = client.unwrap();

    heap_delete<TCPClientObj>(obj);
}

void TCPClient::poll()
{
    LD_PROFILE_SCOPE;

    mObj->io.poll();
}

void TCPClient::connect(const char* host, uint16_t port)
{
    asio::ip::basic_resolver_results<asio::ip::tcp> endpoints = mObj->resolver.resolve(host, std::to_string(port));

    mObj->conn.async_connect(endpoints);
}

void TCPClient::send(View bytes)
{
    mObj->conn.async_write(bytes);
}

void TCPClient::set_user(void* user)
{
    mObj->conn.user = user;
}

void TCPClient::set_on_recv(TransportByteFn onRecv)
{
    mObj->conn.onRecv = onRecv;
}

void TCPClient::set_on_connect(TransportEndpointFn fn)
{
    mObj->conn.onConnect = fn;
}

} // namespace LD
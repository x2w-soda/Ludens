#include <Ludens/DSA/Vector.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/NetworkTransport/TCPServer.h>
#include <Ludens/Profiler/Profiler.h>

#include "TCPConnection.h"

namespace LD {

struct TCPServerObj
{
    asio::io_context io = {};
    tcp::acceptor acceptor;
    TCPConnection conn;

    TCPServerObj(uint16_t port);

    void start_accept();
};

TCPServerObj::TCPServerObj(uint16_t port)
    : acceptor(io, tcp::endpoint(tcp::v4(), port)), conn(io)
{
    start_accept();
}

void TCPServerObj::start_accept()
{
    conn.async_accept(acceptor);
}

//
// Public API
//

TCPServer TCPServer::create(uint16_t port)
{
    auto* obj = heap_new<TCPServerObj>(MEMORY_USAGE_NETWORK, port);

    return TCPServer(obj);
}

void TCPServer::destroy(TCPServer server)
{
    auto* obj = server.unwrap();

    heap_delete<TCPServerObj>(obj);
}

void TCPServer::poll()
{
    LD_PROFILE_SCOPE;

    mObj->io.poll();
}

void TCPServer::send(View bytes)
{
    mObj->conn.async_write(bytes);
}

void TCPServer::set_user(void* user)
{
    mObj->conn.user = user;
}

void TCPServer::set_on_recv(TransportByteFn fn)
{
    mObj->conn.onRecv = fn;
}

} // namespace LD
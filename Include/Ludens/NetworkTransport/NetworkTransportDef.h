#pragma once

#include <Ludens/DSA/String.h>
#include <Ludens/Header/View.h>

namespace LD {

struct TransportEndpoint
{
    String host;
    uint16_t port;
};

/// @brief The callback delivers bytes as a transient byte view
///        and has no notion of packet framing.
typedef void (*TransportByteFn)(View bytes, void* user);

/// @brief The callback notifies connection status with an endpoint.
typedef void (*TransportEndpointFn)(const TransportEndpoint& endpoint, void* user);

} // namespace LD
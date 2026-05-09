#pragma once

namespace LD {

struct JSONRPCMessage;

enum JSONRPCMessageType
{
    JSON_RPC_MESSAGE_REQUEST,
    JSON_RPC_MESSAGE_RESPONSE,
    JSON_RPC_MESSAGE_NOTIFICATION, // a request without ID
};

typedef void (*JSONRPCMessageFn)(const JSONRPCMessage& msg, void* user);

} // namespace LD
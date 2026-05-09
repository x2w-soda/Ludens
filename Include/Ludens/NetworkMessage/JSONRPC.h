#pragma once

#include <Ludens/DSA/String.h>
#include <Ludens/Header/View.h>
#include <Ludens/NetworkMessage/JSONRPCDef.h>
#include <Ludens/Serial/Value.h>

namespace LD {

struct JSONRPCMessage
{
    JSONRPCMessageType type;
    Value64 id;
    String method;
    View payload;
};

class JSONRPCParser
{
public:
    void set_on_message(JSONRPCMessageFn onMessage, void* user);

    /// @brief Triggers message callback when a complete PRC message is parsed.
    /// @param data Accumulative data, may be incomplete JSON from transport layer.
    void append(View data);

private:
    size_t parse_content_length(View header);
    void parse_message(View json);
    String parse_string_value(View json, size_t pos);
    Value64 parse_id_value(View json, size_t pos);
    View parse_json_payload(View json, View key);

private:
    String mBuffer;
    JSONRPCMessageFn mOnMessage = nullptr;
    void* mUser = nullptr;
    size_t mContentLength = 0;
    bool mIsReadingLength = true;
};

} // namespace LD
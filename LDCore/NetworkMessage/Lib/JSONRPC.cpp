#include <Ludens/NetworkMessage/JSONRPC.h>

#include <charconv>

namespace LD {

void JSONRPCParser::set_on_message(JSONRPCMessageFn onMessage, void* user)
{
    mUser = user;
    mOnMessage = onMessage;
}

void JSONRPCParser::append(View data)
{
    mBuffer.append((const char8_t*)data.data, data.size);

    while (true)
    {
        if (mIsReadingLength)
        {
            size_t delimiterPos = mBuffer.find("\r\n\r\n");
            if (delimiterPos == String::npos)
                return;

            View headerV(mBuffer.data(), delimiterPos);
            mContentLength = parse_content_length(headerV);

            mBuffer.erase(0, delimiterPos + 4);
            mIsReadingLength = false;
        }

        if (!mIsReadingLength)
        {
            if (mBuffer.size() < mContentLength)
                return; // wait for more data

            // TODO: handle RPC batch call
            parse_message(View(mBuffer.data(), mContentLength));

            mBuffer.erase(0, mContentLength);
            mIsReadingLength = true;
        }
    }
}

size_t JSONRPCParser::parse_content_length(View header)
{
    constexpr char marker[] = "Content-Length: ";
    constexpr size_t markerLen = sizeof(marker) - 1;

    size_t start = header.find(marker, markerLen);
    if (start == View::npos)
        return 0;

    size_t length = 0;
    size_t valuePos = start + markerLen;
    std::from_chars((const char*)header.data + valuePos, (const char*)header.data + header.size, length);
    return length;
}

void JSONRPCParser::parse_message(View json)
{
    // minimum protocol validation, check for "jsonrpc":"2.0"
    size_t protocolPos = json.find("\"jsonrpc\"");
    if (protocolPos == View::npos || json.find("\"2.0\"", 5, protocolPos) == View::npos)
        return;

    JSONRPCMessage msg{};
    msg.type = JSON_RPC_MESSAGE_NOTIFICATION;

    constexpr char methodKey[] = "\"method\"";
    constexpr size_t methodKeyLen = sizeof(methodKey) - 1;
    size_t methodKeyPos = json.find(methodKey, methodKeyLen);
    if (methodKeyPos != View::npos)
    {
        msg.method = parse_string_value(json, methodKeyPos);
    }

    constexpr char idKey[] = "\"id\"";
    constexpr size_t idKeyLen = sizeof(idKey) - 1;
    size_t idKeyPos = json.find(idKey, idKeyLen);
    if (idKeyPos != View::npos)
    {
        msg.id = parse_id_value(json, idKeyPos);
        msg.type = methodKeyPos == View::npos ? JSON_RPC_MESSAGE_RESPONSE : JSON_RPC_MESSAGE_REQUEST;
    }

    msg.payload = parse_json_payload(json, "\"params\"");

    if (!msg.payload)
        msg.payload = parse_json_payload(json, "\"result\"");

    if (!msg.payload)
        msg.payload = parse_json_payload(json, "\"error\"");

    if (mOnMessage)
        mOnMessage(msg, mUser);
}

String JSONRPCParser::parse_string_value(View json, size_t pos)
{
    // TODO: handle escaped double quote
    size_t start = json.find(":", 1, pos);
    size_t quoteStart = json.find("\"", 1, start);
    size_t quoteEnd = json.find("\"", 1, quoteStart + 1);
    
    if (quoteStart == View::npos || quoteEnd == View::npos)
        return {};

    return String(json.data + quoteStart + 1, quoteEnd - quoteStart - 1);
}

Value64 JSONRPCParser::parse_id_value(View json, size_t pos)
{
    Value64 value{};
    size_t start = json.find(":", 1, pos);
    size_t valStart = json.find_first_not_of(" :", 2, start);

    if (json.data[valStart] == '\"')
    {
        size_t valEnd = json.find("\"", valStart + 1);
        value.set_string(String((const char*)json.data + valStart + 1, valEnd - valStart - 1));
        return value;
    }

    uint64_t u64 = 0;
    std::from_chars((const char*)json.data + valStart, (const char*)json.data + json.size, u64);
    value.set_u64(u64);

    return value;
}

View JSONRPCParser::parse_json_payload(View json, View key)
{
    size_t keyPos = json.find(key.data, key.size);
    if (keyPos == View::npos)
        return {};

    size_t startPos = json.find_first_of("{[", 2, keyPos);
    if (startPos == View::npos)
        return {};

    int depth = 0;

    // NOTE: This is just wrong if input json is invalid.
    //       We are currently accepting something like "{[}]"
    for (size_t i = startPos; i < json.size; i++)
    {
        if (json.data[i] == '{' || json.data[i] == '[')
            depth++;
        else if (json.data[i] == '}' || json.data[i] == ']')
            depth--;

        if (depth == 0)
            return View(json.data + startPos, i - startPos + 1);
    }

    return {};
}

} // namespace LD
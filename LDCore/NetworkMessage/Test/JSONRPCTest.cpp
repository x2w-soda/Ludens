#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>
#include <Ludens/NetworkMessage/JSONRPC.h>

using namespace LD;

struct JSONRPCMessageTest
{
    TView<JSONRPCMessage> expected = {};
    size_t index = 0;

    inline bool passed() const
    {
        return index == expected.size;
    }

    static void validate(const JSONRPCMessage& actual, void* user)
    {
        auto* test = (JSONRPCMessageTest*)user;

        const JSONRPCMessage& expected = test->expected.data[test->index++];
        CHECK(actual.type == expected.type);
        CHECK(actual.id == expected.id);
        CHECK(actual.method == expected.method);

        if (expected.payload)
            CHECK(actual.payload == expected.payload);
        else
            CHECK_FALSE(actual.payload);
    }
};

TEST_CASE("JSONRPCParser")
{
    JSONRPCParser parser;

    const char msg1[] = R"({"jsonrpc": "2.0", "method": "subtract", "params": [42, 23], "id": 1})";
    const char msg2[] = R"({"jsonrpc": "2.0", "result": [19], "id": 1})";

    JSONRPCMessage expected[] = {
        {JSON_RPC_MESSAGE_REQUEST, Value64((uint64_t)1), "subtract", "[42, 23]"},
        {JSON_RPC_MESSAGE_RESPONSE, Value64((uint64_t)1), {}, "[19]"},
    };

    JSONRPCMessageTest test;
    test.expected.data = expected;
    test.expected.size = sizeof(expected) / sizeof(*expected);

    parser.set_on_message(&JSONRPCMessageTest::validate, &test);
    parser.append(std::format("Content-Length: {}\r\n\r\n", strlen(msg1)));
    parser.append(msg1);
    parser.append(std::format("Content-Length: {}\r\n\r\n", strlen(msg2)));
    parser.append(msg2);
    CHECK(test.passed());
}
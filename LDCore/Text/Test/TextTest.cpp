#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>
#include <Ludens/Text/TextBuffer.h>

using namespace LD;

TEST_CASE("TextBuffer")
{
    TextBuffer<char> buf = TextBuffer<char>::create();

    std::string str = buf.to_string();
    CHECK(str.empty());

    buf.set_string(View{});
    str = buf.to_string();
    CHECK(str.empty());

    buf.set_string((const char*)nullptr);
    str = buf.to_string();
    CHECK(str.empty());

    buf.clear();
    str = buf.to_string();
    CHECK(str.empty());

    buf.set_string("test string");
    str = buf.to_string();
    CHECK(str == "test string");

    buf.clear();
    str = buf.to_string();
    CHECK(str.empty());

    TextBuffer<char>::destroy(buf);
}
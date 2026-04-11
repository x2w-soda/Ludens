#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>
#include <Ludens/Text/Text.h>
#include <Ludens/Text/TextBuffer.h>

using namespace LD;

TEST_CASE("TextBuffer")
{
    TextBuffer buf = TextBuffer::create();

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

    TextBuffer::destroy(buf);
}

TEST_CASE("text_find_previous_word")
{
    const char* str = "this is a sentence";

    size_t pos;
    View v(str);

    pos = text_find_previous_word({}, 0);
    CHECK(pos == 0);

    pos = text_find_previous_word({}, 1);
    CHECK(pos == 0);

    pos = text_find_previous_word(v, 4);
    CHECK(pos == 0);

    pos = text_find_previous_word(v, 5);
    CHECK(pos == 5);

    pos = text_find_previous_word(v, 6);
    CHECK(pos == 5);
}
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>
#include <Ludens/Text/Text.h>
#include <Ludens/Text/TextBuffer.h>

using namespace LD;

TEST_CASE("TextBuffer")
{
    TextBuffer buf = TextBuffer::create();

    String str = buf.to_string();
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
    View v(str, strlen(str));

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

TEST_CASE("text_split_ranges without delim")
{
    std::vector<Range> ranges;

    ranges = text_split_ranges({}, '/');
    CHECK(ranges.empty());
    ranges = text_split_ranges(View(""), '/');
    CHECK(ranges.empty());

    ranges = text_split_ranges(View("path/to/some/file"), '/');
    CHECK(ranges.size() == 4);
    CHECK(ranges[0] == Range(0, 4));
    CHECK(ranges[1] == Range(5, 2));
    CHECK(ranges[2] == Range(8, 4));
    CHECK(ranges[3] == Range(13, 4));

    ranges = text_split_ranges(View("path.to.some.file"), '/');
    CHECK(ranges.size() == 1);
    CHECK(ranges[0] == Range(0, 17));

    ranges = text_split_ranges(View("/////"), '/');
    CHECK(ranges.empty());
    ranges = text_split_ranges(View("//text//here///hi"), '/');
    CHECK(ranges.size() == 3);
    CHECK(ranges[0] == Range(2, 4));
    CHECK(ranges[1] == Range(8, 4));
    CHECK(ranges[2] == Range(15, 2));
}

TEST_CASE("text_split_ranges with delim")
{
    std::vector<Range> ranges;

    ranges = text_split_ranges({}, '/', true);
    CHECK(ranges.empty());
    ranges = text_split_ranges(View(""), '/', true);
    CHECK(ranges.empty());

    ranges = text_split_ranges(View("path/to/some/file"), '/', true);
    CHECK(ranges.size() == 7);
    CHECK(ranges[0] == Range(0, 4));
    CHECK(ranges[1] == Range(4, 1));
    CHECK(ranges[2] == Range(5, 2));
    CHECK(ranges[3] == Range(7, 1));
    CHECK(ranges[4] == Range(8, 4));
    CHECK(ranges[5] == Range(12, 1));
    CHECK(ranges[6] == Range(13, 4));

    ranges = text_split_ranges(View("path.to.some.file"), '/', true);
    CHECK(ranges.size() == 1);
    CHECK(ranges[0] == Range(0, 17));

    ranges = text_split_ranges(View("/////"), '/', true);
    CHECK(ranges.size() == 5);
    CHECK(ranges[0] == Range(0, 1));
    CHECK(ranges[1] == Range(1, 1));
    CHECK(ranges[2] == Range(2, 1));
    CHECK(ranges[3] == Range(3, 1));
    CHECK(ranges[4] == Range(4, 1));
    ranges = text_split_ranges(View("//text//here///hi"), '/', true);
    CHECK(ranges.size() == 10);
    CHECK(ranges[0] == Range(0, 1));
    CHECK(ranges[1] == Range(1, 1));
    CHECK(ranges[2] == Range(2, 4));
    CHECK(ranges[3] == Range(6, 1));
    CHECK(ranges[4] == Range(7, 1));
    CHECK(ranges[5] == Range(8, 4));
    CHECK(ranges[6] == Range(12, 1));
    CHECK(ranges[7] == Range(13, 1));
    CHECK(ranges[8] == Range(14, 1));
    CHECK(ranges[9] == Range(15, 2));
}
#include <Extra/doctest/doctest.h>
#include <Ludens/DSA/GapBuffer.h>

using namespace LD;

TEST_CASE("GapBuffer")
{
    GapBuffer<char> buf;
    std::string str = buf.to_string();    

    CHECK(buf.size() == 0);
    CHECK(str.empty());

    buf.insert(0, 'H');
    str = buf.to_string();

    CHECK(buf.size() == 1);
    CHECK(str == "H");
    CHECK(buf.at(0) == 'H');

    buf.at(0) = 'h';
    CHECK(buf.at(0) == 'h');

    // insert cstr
    buf.insert(1, "ello!");
    str = buf.to_string();
    CHECK(str == "hello!");

    // NOP
    buf.insert(1, nullptr);
    str = buf.to_string();
    CHECK(str == "hello!");

    // insert STL string
    buf.insert(5, std::string(", world"));
    str = buf.to_string();
    CHECK(str == "hello, world!");

    buf.erase(4, 2);
    str = buf.to_string();
    CHECK(str == "hell world!");

    // NOP
    buf.erase(4, 0);
    str = buf.to_string();
    CHECK(str == "hell world!");

    buf.erase(0, 1000);
    str = buf.to_string();
    CHECK(str.empty());
    CHECK(buf.size() == 0);
}
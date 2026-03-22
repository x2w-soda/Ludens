#include <Extra/doctest/doctest.h>
#include <Ludens/Header/View.h>

using namespace LD;

// Views should still be POD
static_assert(std::is_trivial_v<View>);
static_assert(std::is_trivial_v<MutView>);

TEST_CASE("View ctor")
{
    View v{};
    CHECK(v.size == 0);
    CHECK(v.data == nullptr);
    CHECK_FALSE(v);

    // equals an empty C string.
    CHECK(v == "");

    // does not equal the absence of C string.
    CHECK_FALSE(v == nullptr);

    v = View("foobar");
    CHECK(v.size == 6);
    CHECK(!memcmp(v.data, "foobar", v.size));

    v = View(nullptr);
    CHECK(v.data == nullptr);
    CHECK(v.size == 0);

    v = View("foobar", 3);
    CHECK(v.size == 3);
    CHECK(!memcmp(v.data, "foo", v.size));

    std::string str("std string");
    v = View(str);
    CHECK(v.size == 10);
    CHECK(!memcmp(v.data, str.data(), v.size));

    const char* cstr = "cstr literal";
    std::string_view strView(cstr);
    v = View(strView);
    CHECK(v.size == 12);
    CHECK(!memcmp(v.data, cstr, v.size));
}

#include <Extra/doctest/doctest.h>
#include <Ludens/Header/Math/Math.h>
#include <Ludens/Media/Format/TOML.h>
#include <Ludens/System/Memory.h>
#include <cstring>
#include <iostream>
#include <string>

using namespace LD;

TEST_CASE("TOML scalars")
{
    TOMLDocument doc = TOMLDocument::create();

    const char toml[] = R"(
b1 = true
b2 = false
s1 = "string value"
f1 = 2.71828
i1 = 1234
)";
    std::string error;
    bool ok = TOMLParser::parse(doc, View(toml, sizeof(toml) - 1), error);
    CHECK(ok);

    bool b;
    TOMLValue v = doc.get("b1");
    CHECK(v);
    CHECK(v.get_bool(b));
    CHECK(b == true);

    v = doc.get("b2");
    CHECK(v);
    CHECK(v.get_bool(b));
    CHECK(b == false);

    std::string string;
    v = doc.get("s1");
    CHECK(v);
    CHECK(v.get_string(string));
    CHECK(string == "string value");

    float f;
    double d;
    v = doc.get("f1");
    CHECK(v);
    CHECK(v.get_f32(f));
    CHECK(is_equal_epsilon(f, 2.71828f));
    CHECK(v.get_f64(d));
    CHECK(is_equal_epsilon(d, 2.71828));

    int32_t i32;
    int64_t i64;
    v = doc.get("i1");
    CHECK(v);
    CHECK(v.get_i32(i32));
    CHECK(i32 == 1234);
    CHECK(v.get_i64(i64));
    CHECK(i64 == 1234);

    TOMLDocument::destroy(doc);
    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}

TEST_CASE("TOML arrays")
{
    TOMLDocument doc = TOMLDocument::create();

    const char toml[] = R"(
a1 = [true, -12345]
a2 = []
)";
    std::string error;
    bool ok = TOMLParser::parse(doc, View(toml, sizeof(toml) - 1), error);
    CHECK(ok);

    {
        TOMLValue a1 = doc.get("a1");
        CHECK(a1);
        CHECK(a1.type() == TOML_TYPE_ARRAY);
        CHECK(a1.size() == 2);

        bool b;
        TOMLValue v = a1[0];
        CHECK(v);
        CHECK(v.get_bool(b));
        CHECK(b == true);

        int i32;
        v = a1[1];
        CHECK(v);
        CHECK(v.get_i32(i32));
        CHECK(i32 == -12345);

        v = a1[-1];
        CHECK_FALSE(v);
        v = a1[2];
        CHECK_FALSE(v);
    }

    {
        TOMLValue a2 = doc.get("a2");
        CHECK(a2);
        CHECK(a2.type() == TOML_TYPE_ARRAY);
        CHECK(a2.size() == 0);
    }

    TOMLDocument::destroy(doc);
    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}

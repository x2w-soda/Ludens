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
    bool ok = doc.parse(toml, strlen(toml), error);
    CHECK(ok);

    bool b;
    TOMLValue v = doc.get("b1");
    CHECK(v);
    CHECK(v.is_bool(b));
    CHECK(b == true);

    v = doc.get("b2");
    CHECK(v);
    CHECK(v.is_bool(b));
    CHECK(b == false);

    std::string string;
    v = doc.get("s1");
    CHECK(v);
    CHECK(v.is_string(string));
    CHECK(string == "string value");

    float e;
    v = doc.get("f1");
    CHECK(v);
    CHECK(v.is_f32(e));
    CHECK(is_equal_epsilon(e, 2.71828f));

    int32_t i32;
    v = doc.get("i1");
    CHECK(v);
    CHECK(v.is_i32(i32));
    CHECK(i32 == 1234);

    float f;
    double d;
    CHECK(v.is_f32(f));
    CHECK(f == 1234.0f);
    CHECK(v.is_f64(d));
    CHECK(d == 1234.0);

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
    bool ok = doc.parse(toml, strlen(toml), error);
    CHECK(ok);

    {
        TOMLValue a1 = doc.get("a1");
        CHECK(a1);
        CHECK(a1.get_type() == TOML_TYPE_ARRAY);
        CHECK(a1.get_size() == 2);

        bool b;
        TOMLValue v = a1[0];
        CHECK(v);
        CHECK(v.is_bool(b));
        CHECK(b == true);

        int i32;
        v = a1[1];
        CHECK(v);
        CHECK(v.is_i32(i32));
        CHECK(i32 == -12345);

        v = a1[-1];
        CHECK_FALSE(v);
        v = a1[2];
        CHECK_FALSE(v);
    }

    {
        TOMLValue a2 = doc.get("a2");
        CHECK(a2);
        CHECK(a2.get_type() == TOML_TYPE_ARRAY);
        CHECK(a2.get_size() == 0);
    }

    TOMLDocument::destroy(doc);
    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}
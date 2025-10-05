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
    CHECK(v.set_bool(true));
    CHECK(v.is_bool(b));
    CHECK(b == true);

    std::string string;
    v = doc.get("s1");
    CHECK(v);
    CHECK(v.is_string(string));
    CHECK(string == "string value");
    CHECK(v.set_string("modified value"));
    CHECK(v.is_string(string));
    CHECK(string == "modified value");

    float f;
    double d;
    v = doc.get("f1");
    CHECK(v);
    CHECK(v.is_f32(f));
    CHECK(is_equal_epsilon(f, 2.71828f));
    CHECK(v.is_f64(d));
    CHECK(is_equal_epsilon(d, 2.71828));
    CHECK(v.set_f64(3.1415));
    CHECK(v.is_f32(f));
    CHECK(is_equal_epsilon(f, 3.1415f));
    CHECK(v.is_f64(d));
    CHECK(is_equal_epsilon(d, 3.1415));

    int32_t i32;
    int64_t i64;
    v = doc.get("i1");
    CHECK(v);
    CHECK(v.is_i32(i32));
    CHECK(i32 == 1234);
    CHECK(v.is_i64(i64));
    CHECK(i64 == 1234);
    CHECK(v.set_i32(5678));
    CHECK(v.is_i32(i32));
    CHECK(i32 == 5678);
    CHECK(v.is_i64(i64));
    CHECK(i64 == 5678);

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

TEST_CASE("TOML table set_key")
{
    TOMLDocument doc = TOMLDocument::create();

    const char toml[] = R"(
[table]
)";
    std::string error;
    bool ok = doc.parse(toml, strlen(toml), error);
    CHECK(ok);

    TOMLValue t = doc.get("table");
    CHECK(t);
    CHECK(t.get_type() == TOML_TYPE_TABLE);
    CHECK(t.get_size() == 0);

    // set new key
    TOMLValue v = t.set_key("foo", TOML_TYPE_INT);
    CHECK(v);
    CHECK(v.get_type() == TOML_TYPE_INT);
    v.set_i32(30);
    TOMLType typeMatch = TOML_TYPE_INT;
    CHECK(t.has_key("foo", &typeMatch));

    // validate changes
    doc.consolidate();
    CHECK(t.get_type() == TOML_TYPE_TABLE);
    CHECK(t.get_size() == 1);
    v = t["foo"];
    int32_t i32;
    CHECK(v);
    CHECK(v.is_i32(i32));
    CHECK(i32 == 30);

    // override existing key
    v = t.set_key("foo", TOML_TYPE_BOOL);
    CHECK(v);
    CHECK(v.get_type() == TOML_TYPE_BOOL);
    CHECK(v.set_bool(true));

    // validate changes
    doc.consolidate();
    CHECK(t.get_size() == 1);
    v = t["foo"];
    bool b;
    CHECK(v);
    CHECK(v.is_bool(b));
    CHECK(b == true);
    typeMatch = TOML_TYPE_BOOL;
    CHECK(t.has_key("foo", &typeMatch));

    TOMLDocument::destroy(doc);
    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}

#include <Extra/doctest/doctest.h>
#include <Ludens/Header/Math/Math.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Memory/Memory.h>

#include <cstring>
#include <iostream>
#include <string>

using namespace LD;

TEST_CASE("TOMLReader scalars")
{
    const char toml[] = R"(
b1 = true
b2 = false
s1 = "string value"
f1 = 2.71828
i1 = 1234
)";
    std::string error;
    TOMLReader reader = TOMLReader::create(View(toml, sizeof(toml) - 1), error);
    CHECK(reader);
    CHECK(error.empty());

    bool b;
    CHECK(reader.read_bool("b1", b));
    CHECK(b == true);

    CHECK(reader.read_bool("b2", b));
    CHECK(b == false);

    std::string str;
    CHECK(reader.read_string("s1", str));
    CHECK(str == "string value");

    float f;
    double d;
    CHECK(reader.read_f32("f1", f));
    CHECK(is_equal_epsilon(f, 2.71828f));
    CHECK(reader.read_f64("f1", d));
    CHECK(is_equal_epsilon(d, 2.71828));

    int32_t i32;
    int64_t i64;
    CHECK(reader.read_i32("i1", i32));
    CHECK(i32 == 1234);
    CHECK(reader.read_i64("i1", i64));
    CHECK(i64 == 1234);

    TOMLReader::destroy(reader);
    reader = {};

    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}

TEST_CASE("TOMLReader arrays")
{
    const char toml[] = R"(
a1 = [true, -12345]
a2 = []
)";
    std::string error;
    TOMLReader reader = TOMLReader::create(View(toml, sizeof(toml) - 1), error);
    CHECK(reader);

    int size;

    {
        CHECK(reader.enter_array("a1", size));
        CHECK(size == 2);

        bool b;
        CHECK(reader.read_bool(0, b));
        CHECK(b == true);

        int i32;
        CHECK(reader.read_i32(1, i32));
        CHECK(i32 == -12345);

        CHECK_FALSE(reader.read_i32(-1, i32));
        CHECK_FALSE(reader.read_i32(2, i32));

        reader.exit();
    }

    {
        CHECK(reader.enter_array("a2", size));
        CHECK(size == 0);
        reader.exit();
    }

    TOMLReader::destroy(reader);
    reader = {};

    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}

TEST_CASE("TOMLUtil Vec2")
{
    TOMLWriter writer = TOMLWriter::create();
    writer.begin();
    {
        Vec2 v(3.141f, -2.718f);
        bool ok = TOMLUtil::write_vec2(writer, "v1", v);
        CHECK(ok);
    }
    std::string toml;
    writer.end(toml);
    TOMLWriter::destroy(writer);

    toml += "v2 = [2, 3.0]\n";          // valid
    toml += "v3 = {y = 5, x = -4.0}\n"; // valid
    toml += "v4 = [3.0]\n";             // invalid

    std::string error;
    TOMLReader reader = TOMLReader::create(View(toml.data(), toml.size()), error);
    {
        CHECK(reader);
        Vec2 v;
        CHECK(TOMLUtil::read_vec2(reader, "v1", v));
        CHECK(is_equal_epsilon(v.x, 3.141f));
        CHECK(is_equal_epsilon(v.y, -2.718f));

        CHECK(TOMLUtil::read_vec2(reader, "v2", v));
        CHECK(is_equal_epsilon(v.x, 2.0f));
        CHECK(is_equal_epsilon(v.y, 3.0f));

        CHECK(TOMLUtil::read_vec2(reader, "v3", v));
        CHECK(is_equal_epsilon(v.x, -4.0f));
        CHECK(is_equal_epsilon(v.y, 5.0f));

        CHECK_FALSE(TOMLUtil::read_vec2(reader, "v4", v));
        CHECK_FALSE(TOMLUtil::read_vec2(reader, "bruh", v));
    }
    TOMLReader::destroy(reader);

    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}

TEST_CASE("TOMLUtil Vec3")
{
    TOMLWriter writer = TOMLWriter::create();
    writer.begin();
    {
        Vec3 v(3.141f, -2.718f, 5.0f);
        bool ok = TOMLUtil::write_vec3(writer, "v1", v);
        CHECK(ok);
    }
    std::string toml;
    writer.end(toml);
    TOMLWriter::destroy(writer);

    toml += "v2 = [2, 3.0, -4]\n";             // valid
    toml += "v3 = {z = 9, y = 5, x = -4.0}\n"; // valid
    toml += "v4 = [3.0, 4.0, false]\n";        // invalid
    toml += "v5 = [3.0, 4.0]\n";               // invalid

    std::string error;
    TOMLReader reader = TOMLReader::create(View(toml.data(), toml.size()), error);
    {
        CHECK(reader);
        Vec3 v;
        CHECK(TOMLUtil::read_vec3(reader, "v1", v));
        CHECK(is_equal_epsilon(v.x, 3.141f));
        CHECK(is_equal_epsilon(v.y, -2.718f));
        CHECK(is_equal_epsilon(v.z, 5.0f));

        CHECK(TOMLUtil::read_vec3(reader, "v2", v));
        CHECK(is_equal_epsilon(v.x, 2.0f));
        CHECK(is_equal_epsilon(v.y, 3.0f));
        CHECK(is_equal_epsilon(v.z, -4.0f));

        CHECK(TOMLUtil::read_vec3(reader, "v3", v));
        CHECK(is_equal_epsilon(v.x, -4.0f));
        CHECK(is_equal_epsilon(v.y, 5.0f));
        CHECK(is_equal_epsilon(v.z, 9.0f));

        CHECK_FALSE(TOMLUtil::read_vec3(reader, "v4", v));
        CHECK_FALSE(TOMLUtil::read_vec3(reader, "v5", v));
        CHECK_FALSE(TOMLUtil::read_vec3(reader, "bruh", v));
    }
    TOMLReader::destroy(reader);

    CHECK_FALSE(get_memory_leaks(nullptr));
}

TEST_CASE("TOMLUtil Rect")
{
    TOMLWriter writer = TOMLWriter::create();
    writer.begin();
    {
        Rect rect(0.0f, 1.0f, 2.0f, 3.0f);
        bool ok = TOMLUtil::write_rect(writer, "r1", rect);
        CHECK(ok);
    }
    std::string toml;
    writer.end(toml);
    TOMLWriter::destroy(writer);

    toml += "r2 = {h = 9, y = 5, x = -4.0, w = 3.14}\n"; // valid
    toml += "r3 = {}\n";                                 // invalid
    toml += "r4 = {x = 1.0, y = 2.0}\n";                 // invalid

    std::string error;
    TOMLReader reader = TOMLReader::create(View(toml.data(), toml.size()), error);
    {
        CHECK(reader);
        Rect rect;
        CHECK(TOMLUtil::read_rect(reader, "r1", rect));
        CHECK(is_equal_epsilon(rect.x, 0.0f));
        CHECK(is_equal_epsilon(rect.y, 1.0f));
        CHECK(is_equal_epsilon(rect.w, 2.0f));
        CHECK(is_equal_epsilon(rect.h, 3.0f));

        CHECK(TOMLUtil::read_rect(reader, "r2", rect));
        CHECK(is_equal_epsilon(rect.x, -4.0f));
        CHECK(is_equal_epsilon(rect.y, 5.0f));
        CHECK(is_equal_epsilon(rect.w, 3.14f));
        CHECK(is_equal_epsilon(rect.h, 9.0f));

        CHECK_FALSE(TOMLUtil::read_rect(reader, "r3", rect));
        CHECK_FALSE(TOMLUtil::read_rect(reader, "r4", rect));
        CHECK_FALSE(TOMLUtil::read_rect(reader, "bruh", rect));
    }
    TOMLReader::destroy(reader);

    CHECK_FALSE(get_memory_leaks(nullptr));
}

TEST_CASE("TOMLUtil Transform2D")
{
    TOMLWriter writer = TOMLWriter::create();
    writer.begin();
    {
        Transform2D tr;
        tr.position = Vec2(2.0f, 3.0f);
        tr.rotation = 45.0f;
        tr.scale = Vec2(1.0f, 4.0f);
        bool ok = TOMLUtil::write_transform_2d(writer, "t1", tr);
        CHECK(ok);
    }
    std::string toml;
    writer.end(toml);
    TOMLWriter::destroy(writer);

    toml += "t2 = { scale = {x=3, y=4}, position = [-2, -3], rotation = -45}\n";
    toml += "t3 = {}\n";

    std::string error;
    TOMLReader reader = TOMLReader::create(View(toml.data(), toml.size()), error);
    {
        CHECK(reader);
        Transform2D tr;
        CHECK(TOMLUtil::read_transform_2d(reader, "t1", tr));
        CHECK(tr.position == Vec2(2.0f, 3.0f));
        CHECK(tr.scale == Vec2(1.0f, 4.0f));
        CHECK(is_equal_epsilon(tr.rotation, 45.0f));

        CHECK(TOMLUtil::read_transform_2d(reader, "t2", tr));
        CHECK(tr.position == Vec2(-2.0f, -3.0f));
        CHECK(tr.scale == Vec2(3.0f, 4.0f));
        CHECK(is_equal_epsilon(tr.rotation, -45.0f));

        CHECK_FALSE(TOMLUtil::read_transform_2d(reader, "t3", tr));
        CHECK_FALSE(TOMLUtil::read_transform_2d(reader, "bruh", tr));
    }
    TOMLReader::destroy(reader);

    CHECK_FALSE(get_memory_leaks(nullptr));
}

TEST_CASE("TOMLUtil Transform")
{
    TOMLWriter writer = TOMLWriter::create();
    writer.begin();
    {
        TransformEx tr;
        tr.position = Vec3(2.0f, 3.0f, 0.0f);
        tr.rotationEuler = Vec3(30.0f, 60.0f, 90.0f);
        tr.scale = Vec3(1.0f, 4.0f, 2.0f);
        bool ok = TOMLUtil::write_transform(writer, "t1", tr);
        CHECK(ok);
    }
    std::string toml;
    writer.end(toml);
    TOMLWriter::destroy(writer);

    toml += "t2 = { scale = {x=3, z=5, y=4}, position = [-2, -3, -4], rotation = [-30, -60.0, +90]}\n";
    toml += "t3 = {}\n";

    std::string error;
    TOMLReader reader = TOMLReader::create(View(toml.data(), toml.size()), error);
    {
        CHECK(reader);
        TransformEx tr;
        CHECK(TOMLUtil::read_transform(reader, "t1", tr));
        CHECK(tr.position == Vec3(2.0f, 3.0f, 0.0f));
        CHECK(tr.rotationEuler == Vec3(30.0f, 60.0f, 90.0f));
        CHECK(tr.scale == Vec3(1.0f, 4.0f, 2.0f));

        CHECK(TOMLUtil::read_transform(reader, "t2", tr));
        CHECK(tr.position == Vec3(-2.0f, -3.0f, -4.0f));
        CHECK(tr.rotationEuler == Vec3(-30.0f, -60.0f, 90.0f));
        CHECK(tr.scale == Vec3(3.0f, 4.0f, 5.0f));

        CHECK_FALSE(TOMLUtil::read_transform(reader, "t3", tr));
        CHECK_FALSE(TOMLUtil::read_transform(reader, "bruh", tr));
    }
    TOMLReader::destroy(reader);

    CHECK_FALSE(get_memory_leaks(nullptr));
}
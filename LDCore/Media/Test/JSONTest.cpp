#include <Extra/doctest/doctest.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Media/Format/JSON.h>
#include <Ludens/Memory/Memory.h>

#include <string>

using namespace LD;

TEST_CASE("JSONReader object")
{
    // from the rapidjson example
    std::string json = R"({"project":"rapidjson","stars":10})";
    std::string error;
    JSONReader reader = JSONReader::create(View(json.data(), json.size()), error);
    CHECK(reader);
    CHECK(reader.enter_root_object());
    CHECK(reader.is_object_scope());

    std::string projectName;
    CHECK(reader.read_string("project", projectName));
    CHECK(projectName == "rapidjson");

    int32_t i32;
    CHECK(reader.read_i32("stars", i32));
    CHECK(i32 == 10);

    reader.exit();
    JSONReader::destroy(reader);

    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}

TEST_CASE("JSONReader array")
{
    int size;
    std::string json = R"([123, false, true, [ "string" ]])";
    std::string error;
    JSONReader reader = JSONReader::create(View(json.data(), json.size()), error);
    CHECK(reader);
    CHECK(reader.enter_root_array(size));
    CHECK(reader.is_array_scope());
    CHECK(size == 4);

    int32_t i32;
    CHECK(reader.read_i32(0, i32));
    CHECK(i32 == 123);

    bool b;
    CHECK(reader.read_bool(1, b));
    CHECK(b == false);
    CHECK(reader.read_bool(2, b));
    CHECK(b == true);

    std::string str;
    CHECK(reader.enter_array(3, size));
    CHECK(size == 1);
    CHECK(reader.read_string(0, str));
    CHECK(str == "string");
    reader.exit();

    reader.exit();
    JSONReader::destroy(reader);

    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}

TEST_CASE("JSONWriter nullptr string")
{
    JSONWriter writer = JSONWriter::create();

    std::string str;
    writer.begin();
    writer.begin_object();
    writer.key("str");
    writer.write_string(nullptr); // should not crash
    writer.end_object();
    writer.end(str);

    JSONWriter::destroy(writer);

    CHECK_FALSE(get_memory_leaks(nullptr));
}

TEST_CASE("JSONUtil Vec2")
{
    JSONWriter writer = JSONWriter::create();
    writer.begin();
    writer.begin_object();
    {
        Vec2 v(3.141f, -2.718f);
        bool ok = JSONUtil::write_vec2(writer, "v1", v);
        CHECK(ok);
    }
    std::string json;
    writer.end_object();
    writer.end(json);
    JSONWriter::destroy(writer);

    REQUIRE(json.ends_with('}'));
    json.pop_back();
    json += R"(, "v2" : [2, 3.0])";              // valid
    json += R"(, "v3" : {"y" : 5, "x" : -4.0})"; // valid
    json += R"(, "v4" : [3.0])";                 // invalid
    json.push_back('}');

    std::string error;
    JSONReader reader = JSONReader::create(View(json.data(), json.size()), error);
    CHECK(reader);
    CHECK(reader.enter_root_object());
    {
        Vec2 v;
        CHECK(JSONUtil::read_vec2(reader, "v1", v));
        CHECK(is_equal_epsilon(v.x, 3.141f));
        CHECK(is_equal_epsilon(v.y, -2.718f));

        CHECK(JSONUtil::read_vec2(reader, "v2", v));
        CHECK(is_equal_epsilon(v.x, 2.0f));
        CHECK(is_equal_epsilon(v.y, 3.0f));

        CHECK(JSONUtil::read_vec2(reader, "v3", v));
        CHECK(is_equal_epsilon(v.x, -4.0f));
        CHECK(is_equal_epsilon(v.y, 5.0f));

        CHECK_FALSE(JSONUtil::read_vec2(reader, "v4", v));
        CHECK_FALSE(JSONUtil::read_vec2(reader, "bruh", v));
    }
    reader.exit();
    JSONReader::destroy(reader);

    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}

TEST_CASE("JSONUtil Vec3")
{
    JSONWriter writer = JSONWriter::create();
    writer.begin();
    writer.begin_object();
    {
        Vec3 v(3.141f, -2.718f, 5.0f);
        bool ok = JSONUtil::write_vec3(writer, "v1", v);
        CHECK(ok);
    }
    std::string json;
    writer.end_object();
    writer.end(json);
    JSONWriter::destroy(writer);

    REQUIRE(json.ends_with('}'));
    json.pop_back();
    json += R"(, "v2" : [2, 3.0, -4])";                   // valid
    json += R"(, "v3" : {"z" : 9, "y" : 5, "x" : -4.0})"; // valid
    json += R"(, "v4" : [3.0, 4.0, false])";              // invalid
    json += R"(, "v5" : [3.0, 4.0])";                     // invalid
    json.push_back('}');

    std::string error;
    JSONReader reader = JSONReader::create(View(json.data(), json.size()), error);
    CHECK(reader);
    CHECK(reader.enter_root_object());
    {
        Vec3 v;
        CHECK(JSONUtil::read_vec3(reader, "v1", v));
        CHECK(is_equal_epsilon(v.x, 3.141f));
        CHECK(is_equal_epsilon(v.y, -2.718f));
        CHECK(is_equal_epsilon(v.z, 5.0f));

        CHECK(JSONUtil::read_vec3(reader, "v2", v));
        CHECK(is_equal_epsilon(v.x, 2.0f));
        CHECK(is_equal_epsilon(v.y, 3.0f));
        CHECK(is_equal_epsilon(v.z, -4.0f));

        CHECK(JSONUtil::read_vec3(reader, "v3", v));
        CHECK(is_equal_epsilon(v.x, -4.0f));
        CHECK(is_equal_epsilon(v.y, 5.0f));
        CHECK(is_equal_epsilon(v.z, 9.0f));

        CHECK_FALSE(JSONUtil::read_vec3(reader, "v4", v));
        CHECK_FALSE(JSONUtil::read_vec3(reader, "v5", v));
        CHECK_FALSE(JSONUtil::read_vec3(reader, "bruh", v));
    }
    reader.exit();
    JSONReader::destroy(reader);

    CHECK_FALSE(get_memory_leaks(nullptr));
}

TEST_CASE("JSONUtil Rect")
{
    JSONWriter writer = JSONWriter::create();
    CHECK(writer.begin());
    CHECK(writer.begin_object());
    {
        Rect rect(0.0f, 1.0f, 2.0f, 3.0f);
        CHECK(JSONUtil::write_rect(writer, "r1", rect));
    }
    std::string json;
    CHECK(writer.end_object());
    CHECK(writer.end(json));
    JSONWriter::destroy(writer);

    REQUIRE(json.ends_with('}'));
    json.pop_back();
    json += R"(, "r2" : {"h" : 9, "y" : 5, "x" : -4.0, "w" : 3.14})"; // valid
    json += R"(, "r3" : {})";                                         // invalid
    json += R"(, "r4" : {"x" : 1.0, "y" : 2.0})";                     // invalid
    json.push_back('}');

    std::string error;
    JSONReader reader = JSONReader::create(View(json.data(), json.size()), error);
    CHECK(reader);
    CHECK(reader.enter_root_object());
    {
        Rect rect;
        CHECK(JSONUtil::read_rect(reader, "r1", rect));
        CHECK(is_equal_epsilon(rect.x, 0.0f));
        CHECK(is_equal_epsilon(rect.y, 1.0f));
        CHECK(is_equal_epsilon(rect.w, 2.0f));
        CHECK(is_equal_epsilon(rect.h, 3.0f));

        CHECK(JSONUtil::read_rect(reader, "r2", rect));
        CHECK(is_equal_epsilon(rect.x, -4.0f));
        CHECK(is_equal_epsilon(rect.y, 5.0f));
        CHECK(is_equal_epsilon(rect.w, 3.14f));
        CHECK(is_equal_epsilon(rect.h, 9.0f));

        CHECK_FALSE(JSONUtil::read_rect(reader, "r3", rect));
        CHECK_FALSE(JSONUtil::read_rect(reader, "r4", rect));
        CHECK_FALSE(JSONUtil::read_rect(reader, "bruh", rect));
    }
    reader.exit();
    JSONReader::destroy(reader);

    CHECK_FALSE(get_memory_leaks(nullptr));
}

TEST_CASE("JSONUtil Transform2D")
{
    JSONWriter writer = JSONWriter::create();
    CHECK(writer.begin());
    CHECK(writer.begin_object());
    {
        Transform2D tr;
        tr.position = Vec2(2.0f, 3.0f);
        tr.rotation = 45.0f;
        tr.scale = Vec2(1.0f, 4.0f);
        bool ok = JSONUtil::write_transform_2d(writer, "t1", tr);
        CHECK(ok);
    }
    std::string json;
    CHECK(writer.end_object());
    CHECK(writer.end(json));
    JSONWriter::destroy(writer);

    REQUIRE(json.ends_with('}'));
    json.pop_back();
    json += R"(, "t2" : {"scale" : {"x":3, "y":4}, "position" : [-2, -3.0], "rotation" : -45})"; // valid
    json += R"(, "t3" : {})";                                                                    // invalid
    json.push_back('}');

    std::string error;
    JSONReader reader = JSONReader::create(View(json.data(), json.size()), error);
    CHECK(reader);
    CHECK(reader.enter_root_object());
    {
        Transform2D tr;
        CHECK(JSONUtil::read_transform_2d(reader, "t1", tr));
        CHECK(tr.position == Vec2(2.0f, 3.0f));
        CHECK(tr.scale == Vec2(1.0f, 4.0f));
        CHECK(is_equal_epsilon(tr.rotation, 45.0f));

        CHECK(JSONUtil::read_transform_2d(reader, "t2", tr));
        CHECK(tr.position == Vec2(-2.0f, -3.0f));
        CHECK(tr.scale == Vec2(3.0f, 4.0f));
        CHECK(is_equal_epsilon(tr.rotation, -45.0f));

        CHECK_FALSE(JSONUtil::read_transform_2d(reader, "t3", tr));
        CHECK_FALSE(JSONUtil::read_transform_2d(reader, "bruh", tr));
    }
    reader.exit();
    JSONReader::destroy(reader);

    CHECK_FALSE(get_memory_leaks(nullptr));
}

TEST_CASE("JSONUtil Transform")
{
    JSONWriter writer = JSONWriter::create();
    CHECK(writer.begin());
    CHECK(writer.begin_object());
    {
        TransformEx tr;
        tr.position = Vec3(2.0f, 3.0f, 0.0f);
        tr.rotationEuler = Vec3(30.0f, 60.0f, 90.0f);
        tr.scale = Vec3(1.0f, 4.0f, 2.0f);
        bool ok = JSONUtil::write_transform(writer, "t1", tr);
        CHECK(ok);
    }
    std::string json;
    CHECK(writer.end_object());
    CHECK(writer.end(json));
    JSONWriter::destroy(writer);

    REQUIRE(json.ends_with('}'));
    json.pop_back();
    json += R"(, "t2" : {"scale" : {"x":3, "z":5, "y":4}, "position" : [-2, -3.0, -4], "rotation" : [-30, -60.0, 90]})";
    json += R"(, "t3" : {})";
    json.push_back('}');

    std::string error;
    JSONReader reader = JSONReader::create(View(json.data(), json.size()), error);
    CHECK(reader);
    CHECK(reader.enter_root_object());
    {
        TransformEx tr;
        CHECK(JSONUtil::read_transform(reader, "t1", tr));
        CHECK(tr.position == Vec3(2.0f, 3.0f, 0.0f));
        CHECK(tr.rotationEuler == Vec3(30.0f, 60.0f, 90.0f));
        CHECK(tr.scale == Vec3(1.0f, 4.0f, 2.0f));

        CHECK(JSONUtil::read_transform(reader, "t2", tr));
        CHECK(tr.position == Vec3(-2.0f, -3.0f, -4.0f));
        CHECK(tr.rotationEuler == Vec3(-30.0f, -60.0f, 90.0f));
        CHECK(tr.scale == Vec3(3.0f, 4.0f, 5.0f));

        CHECK_FALSE(JSONUtil::read_transform(reader, "t3", tr));
        CHECK_FALSE(JSONUtil::read_transform(reader, "bruh", tr));
    }
    reader.exit();
    JSONReader::destroy(reader);

    CHECK_FALSE(get_memory_leaks(nullptr));
}
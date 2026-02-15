#include <Extra/doctest/doctest.h>
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

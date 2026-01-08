#include <Extra/doctest/doctest.h>
#include <Ludens/Media/Format/JSON.h>
#include <Ludens/System/Memory.h>
#include <string>

using namespace LD;

TEST_CASE("JSON object")
{
    // from the rapidjson example
    const char json[] = R"({"project":"rapidjson","stars":10})";
    JSONDocument doc = JSONDocument::create();
    std::string error;
    bool success = JSONParser::parse(doc, View(json, sizeof(json) - 1), error);
    CHECK(success);

    JSONValue root = doc.get_root();
    CHECK(root.is_object());

    std::string projectName;
    JSONValue project = root.get_member("project");
    CHECK(project);
    CHECK(project.is_string(&projectName));
    CHECK(projectName == "rapidjson");

    JSONValue stars = root.get_member("stars");
    CHECK(stars);
    CHECK(stars.is_number());

    int32_t i32;
    CHECK(stars.is_i32(&i32));
    CHECK(i32 == 10);

    JSONDocument::destroy(doc);

    MemoryProfile profile = get_memory_profile(MEMORY_USAGE_SERIAL);
    CHECK(profile.current == 0);
}

TEST_CASE("JSON array")
{
    const char json[] = R"([123, false, true, [ "string" ]])";
    JSONDocument doc = JSONDocument::create();
    std::string error;
    bool success = JSONParser::parse(doc, View(json, sizeof(json) - 1), error);
    CHECK(success);

    JSONValue root = doc.get_root();
    CHECK(root.is_array());
    CHECK(root.size() == 4);

    int32_t i32;
    JSONValue element = root.get_index(0);
    CHECK(element);
    CHECK(element.is_i32(&i32));
    CHECK(i32 == 123);

    element = root.get_index(1);
    CHECK(element);
    CHECK(element.is_false());

    element = root.get_index(2);
    CHECK(element);
    CHECK(element.is_true());

    JSONValue array = root.get_index(3);
    CHECK(array);
    CHECK(array.is_array());
    CHECK(array.size() == 1);

    CHECK_FALSE(root.get_index(4));

    std::string str;
    element = array.get_index(0);
    CHECK(element);
    CHECK(element.is_string(&str));
    CHECK(str == "string");
    CHECK_FALSE(array.get_index(1));

    JSONDocument::destroy(doc);

    MemoryProfile profile = get_memory_profile(MEMORY_USAGE_SERIAL);
    CHECK(profile.current == 0);
}

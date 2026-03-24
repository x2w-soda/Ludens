#include <Extra/doctest/doctest.h>

#include "DocumentBuilderTest.h"

using namespace LD;

TEST_CASE("DocumentRefs")
{
    Document doc = require_document(R"(
[](doc://LuaAPI/LuaScripting.md)
[](doc://Manual/GettingStarted.md)
[](scheme://uri)
)",
                                   "doc://test.md");
    CHECK(doc.get_uri() == "doc://test.md");
    
    DocumentRefs refs = doc.get_references();
    CHECK(refs.luaAPI.size() == 1);
    CHECK(refs.luaAPI[0] == "doc://LuaAPI/LuaScripting.md");
    CHECK(refs.manual.size() == 1);
    CHECK(refs.manual[0] == "doc://Manual/GettingStarted.md");
    CHECK(refs.misc.size() == 1);
    CHECK(refs.misc[0] == "scheme://uri");

    Document::destroy(doc);
    CHECK(get_memory_leaks(nullptr) == 0);
}

TEST_CASE("DocumentRefs Deduplication")
{
    Document doc = require_document(R"(
[](doc://LuaAPI/LuaScripting.md)
[](doc://Manual/GettingStarted.md)
[](doc://LuaAPI/LuaScripting.md)
[](doc://Manual/GettingStarted.md)
[](scheme://uri)
[](scheme://uri)
[](doc://LuaAPI/LuaScripting.md)
)",
                                   "doc://test.md");
    CHECK(doc.get_uri() == "doc://test.md");

    DocumentRefs refs = doc.get_references();
    CHECK(refs.luaAPI.size() == 1);
    CHECK(refs.luaAPI[0] == "doc://LuaAPI/LuaScripting.md");
    CHECK(refs.manual.size() == 1);
    CHECK(refs.manual[0] == "doc://Manual/GettingStarted.md");
    CHECK(refs.misc.size() == 1);
    CHECK(refs.misc[0] == "scheme://uri");

    Document::destroy(doc);
    CHECK(get_memory_leaks(nullptr) == 0);
}
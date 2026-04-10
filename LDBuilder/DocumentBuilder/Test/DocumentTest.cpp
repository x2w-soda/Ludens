#include <Extra/doctest/doctest.h>

#include "DocumentBuilderTest.h"

using namespace LD;

TEST_CASE("DocumentRefs")
{
    Document doc = require_document(R"(
[](ld://Doc/LuaAPI/LuaScripting.md)
[](ld://Doc/Manual/GettingStarted.md)
[](scheme://uri)
)",
                                    "ld://Doc/Test.md");
    CHECK(doc.get_uri() == "ld://Doc/Test.md");

    DocumentRefs refs = doc.get_references();
    CHECK(refs.luaAPI.size() == 1);
    CHECK(refs.luaAPI[0] == "ld://Doc/LuaAPI/LuaScripting.md");
    CHECK(refs.manual.size() == 1);
    CHECK(refs.manual[0] == "ld://Doc/Manual/GettingStarted.md");
    CHECK(refs.misc.size() == 1);
    CHECK(refs.misc[0] == "scheme://uri");

    Document::destroy(doc);
    CHECK(get_memory_leaks(nullptr) == 0);
}

TEST_CASE("DocumentRefs Deduplication")
{
    Document doc = require_document(R"(
[](ld://Doc/LuaAPI/LuaScripting.md)
[](ld://Doc/Manual/GettingStarted.md)
[](ld://Doc/LuaAPI/LuaScripting.md)
[](ld://Doc/Manual/GettingStarted.md)
[](scheme://uri)
[](scheme://uri)
[](ld://Doc/LuaAPI/LuaScripting.md)
)",
                                    "ld://Doc/test.md");
    CHECK(doc.get_uri() == "ld://Doc/test.md");

    DocumentRefs refs = doc.get_references();
    CHECK(refs.luaAPI.size() == 1);
    CHECK(refs.luaAPI[0] == "ld://Doc/LuaAPI/LuaScripting.md");
    CHECK(refs.manual.size() == 1);
    CHECK(refs.manual[0] == "ld://Doc/Manual/GettingStarted.md");
    CHECK(refs.misc.size() == 1);
    CHECK(refs.misc[0] == "scheme://uri");

    Document::destroy(doc);
    CHECK(get_memory_leaks(nullptr) == 0);
}
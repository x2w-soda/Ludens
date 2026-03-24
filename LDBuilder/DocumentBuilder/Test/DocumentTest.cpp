#include <Extra/doctest/doctest.h>

#include "DocumentBuilderTest.h"

using namespace LD;

TEST_CASE("DocumentRefs")
{
    Document doc = create_document(R"(
[](doc://api/LuaScripting.md)
[](doc://manual/GettingStarted.md)
[](scheme://uri)
)",
                                   "doc://test.md");
    CHECK(doc.get_uri() == "doc://test.md");
    
    DocumentRefs refs = doc.get_references();
    CHECK(refs.api.size() == 1);
    CHECK(refs.api[0] == "doc://api/LuaScripting.md");
    CHECK(refs.manual.size() == 1);
    CHECK(refs.manual[0] == "doc://manual/GettingStarted.md");
    CHECK(refs.misc.size() == 1);
    CHECK(refs.misc[0] == "scheme://uri");

    Document::destroy(doc);
    CHECK(get_memory_leaks(nullptr) == 0);
}

TEST_CASE("DocumentRefs Deduplication")
{
    Document doc = create_document(R"(
[](doc://api/LuaScripting.md)
[](doc://manual/GettingStarted.md)
[](doc://api/LuaScripting.md)
[](doc://manual/GettingStarted.md)
[](scheme://uri)
[](scheme://uri)
[](doc://api/LuaScripting.md)
)",
                                   "doc://test.md");
    CHECK(doc.get_uri() == "doc://test.md");

    DocumentRefs refs = doc.get_references();
    CHECK(refs.api.size() == 1);
    CHECK(refs.api[0] == "doc://api/LuaScripting.md");
    CHECK(refs.manual.size() == 1);
    CHECK(refs.manual[0] == "doc://manual/GettingStarted.md");
    CHECK(refs.misc.size() == 1);
    CHECK(refs.misc[0] == "scheme://uri");

    Document::destroy(doc);
    CHECK(get_memory_leaks(nullptr) == 0);
}
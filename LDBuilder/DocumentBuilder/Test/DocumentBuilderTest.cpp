#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>

#include "DocumentBuilderTest.h"

LD::Document create_document(const char* md, const char* uri)
{
    std::string err;
    LD::DocumentInfo info{};
    info.md = LD::View(md, strlen(md));
    info.uri = uri;
    LD::Document doc = LD::Document::create(info, err);
    REQUIRE(doc);

    return doc;
}
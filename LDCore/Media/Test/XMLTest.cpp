#include <Extra/doctest/doctest.h>
#include <Ludens/Media/Format/XML.h>
#include <cstring>

using namespace LD;

// without attributes
// without mixed content strings
TEST_CASE("XML basic")
{
    XMLDocument doc = XMLDocument::create();

    const char xml[] = R"(<h1><b>some text</b></h1>)";
    bool ok = doc.parse(xml, sizeof(xml));
    CHECK(ok);

    XMLElement root = doc.get_root();
    CHECK(root.get_name() == "h1");

    XMLString mixed;
    XMLElement b = root.get_child(mixed);
    CHECK(mixed.size == 0);
    CHECK(!b.get_child(mixed));
    CHECK(mixed == "some text");

    XMLDocument::destroy(doc);
}

TEST_CASE("XML attribute")
{
    XMLDocument doc = XMLDocument::create();

    const char xml[] = R"(<member refid="1234" kind="function"></member>)";
    bool ok = doc.parse(xml, sizeof(xml));
    CHECK(ok);

    XMLElement root = doc.get_root();
    CHECK(root.get_name() == "member");

    XMLAttribute attr = root.get_attributes();
    CHECK(attr.get_name() == "refid");
    CHECK(attr.get_value() == "1234");

    attr = attr.get_next();
    CHECK(attr.get_name() == "kind");
    CHECK(attr.get_value() == "function");

    XMLDocument::destroy(doc);
}

TEST_CASE("XML mixed content")
{
    XMLDocument doc = XMLDocument::create();

    const char xml1[] = R"(<p>This is an example of <i>mixed content</i></p>)";
    bool ok = doc.parse(xml1, sizeof(xml1));
    CHECK(ok);

    XMLElement root = doc.get_root();
    CHECK(root.get_name() == "p");

    XMLString mixed;
    XMLElement i = root.get_child(mixed);
    CHECK(mixed == "This is an example of ");
    CHECK(i.get_name() == "i");

    CHECK(!i.get_child(mixed));
    CHECK(mixed == "mixed content");

    const char xml2[] = R"(< p > some<b> bold</ b> text<i> italic</ i> end</ p>)";
    ok = doc.parse(xml2, sizeof(xml2));
    CHECK(ok);

    root = doc.get_root();
    CHECK(root.get_name() == "p");

    XMLElement tag = root.get_child(mixed);
    CHECK(mixed == " some");
    CHECK(tag.get_name() == "b");
    CHECK(!tag.get_child(mixed));
    CHECK(mixed == " bold");

    tag = tag.get_next(mixed);
    CHECK(mixed == " text");
    CHECK(tag.get_name() == "i");
    CHECK(!tag.get_child(mixed));
    CHECK(mixed == " italic");

    tag = tag.get_next(mixed);
    CHECK(!tag);
    CHECK(mixed == " end");

    XMLDocument::destroy(doc);
}

// XML declaration tag
TEST_CASE("XML declaration")
{
    XMLDocument doc = XMLDocument::create();

    const char xml[] = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?><root></root>)";
    bool ok = doc.parse(xml, sizeof(xml));
    CHECK(ok);

    XMLAttribute attr = doc.get_declaration();
    CHECK(attr.get_name() == "version");
    CHECK(attr.get_value() == "1.0");
    attr = attr.get_next();
    CHECK(attr.get_name() == "encoding");
    CHECK(attr.get_value() == "UTF-8");
    attr = attr.get_next();
    CHECK(attr.get_name() == "standalone");
    CHECK(attr.get_value() == "yes");

    XMLDocument::destroy(doc);
}
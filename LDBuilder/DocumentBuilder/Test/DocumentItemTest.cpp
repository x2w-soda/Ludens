#include <Extra/doctest/doctest.h>
#include <Ludens/Memory/Memory.h>
#include <LudensBuilder/DocumentBuilder/Document.h>

#include "DocumentBuilderTest.h"

using namespace LD;

// early testing, needs some generalization later.
TEST_CASE("DocumentItem headings")
{
    Document doc = create_document(R"(
# heading 1
## heading 2
### heading 3
#### heading 4
##### heading 5
###### heading 6
)");
    auto items = doc.get_items();
    CHECK(items.size == 6);

    for (int i = 1; i <= 6; i++)
    {
        DocumentItem* item = items.data[i - 1];
        CHECK(item->type == DOCUMENT_ITEM_HEADING);
        CHECK(item->spans.size == 1);
        DocumentItemHeading* heading = (DocumentItemHeading*)item;
        CHECK(heading->level == i);

        std::string expected = std::format("heading {}", i);
        DocumentSpan* span = item->spans.data[0];
        CHECK(span->type == DOCUMENT_SPAN_TEXT);
        CHECK(span->text == expected.c_str());
    }

    Document::destroy(doc);

    CHECK(get_memory_leaks(nullptr) == 0);
}

TEST_CASE("DocumentItem paragraph")
{
    Document doc = create_document(R"(
first paragraph

second paragraph
with some more
line breaks
)");
    auto items = doc.get_items();
    CHECK(items.size == 2);

    DocumentItem* item = items.data[0];
    CHECK(item->type == DOCUMENT_ITEM_PARAGRAPH);
    CHECK(item->spans.size == 1);

    DocumentSpan* span = item->spans.data[0];
    CHECK(span->type == DOCUMENT_SPAN_TEXT);
    CHECK(span->text == "first paragraph");

    // line break not strictly preserved, but we should have multiple spans in paragraph
    item = items.data[1];
    CHECK(item->type == DOCUMENT_ITEM_PARAGRAPH);
    CHECK(item->spans.size == 3);

    span = item->spans.data[0];
    CHECK(span->type == DOCUMENT_SPAN_TEXT);
    CHECK(span->text == "second paragraph");
    span = item->spans.data[1];
    CHECK(span->type == DOCUMENT_SPAN_TEXT);
    CHECK(span->text == "with some more");
    span = item->spans.data[2];
    CHECK(span->type == DOCUMENT_SPAN_TEXT);
    CHECK(span->text == "line breaks");

    Document::destroy(doc);
    CHECK(get_memory_leaks(nullptr) == 0);
}
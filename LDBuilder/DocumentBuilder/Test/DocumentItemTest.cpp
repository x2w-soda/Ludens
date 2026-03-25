#include <Extra/doctest/doctest.h>
#include <Ludens/Memory/Memory.h>
#include <LudensBuilder/DocumentBuilder/Document.h>

#include "DocumentBuilderTest.h"

using namespace LD;

// early testing, needs some generalization later.
TEST_CASE("DocumentItem headings")
{
    Document doc = require_document(R"(
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
    Document doc = require_document(R"(
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

TEST_CASE("DocumentItem fenced code block")
{
    Document doc = require_document(R"(
```python
if __name__ == '__main__':
    print('hello world')
```)");
    auto items = doc.get_items();
    CHECK(items.size == 1);

    DocumentItemCodeBlock* block = (DocumentItemCodeBlock*)items.data[0];
    CHECK(block->item.type == DOCUMENT_ITEM_CODE_BLOCK);
    CHECK(block->lang == "python");

    TView<DocumentSpan*> spans = block->item.spans;
    CHECK(spans.size == 1);
    CHECK(spans.data[0]->type == DOCUMENT_SPAN_TEXT);
    CHECK(spans.data[0]->text == "if __name__ == '__main__':\n    print('hello world')\n");

    Document::destroy(doc);
    CHECK(get_memory_leaks(nullptr) == 0);
}

TEST_CASE("DocumentItem indented code block with tabs")
{
    Document doc = require_document("\tindented\n\tcode\n\tblock via\n\ttabs\n");
    auto items = doc.get_items();
    CHECK(items.size == 1);

    DocumentItemCodeBlock* block = (DocumentItemCodeBlock*)items.data[0];
    CHECK(block->item.type == DOCUMENT_ITEM_CODE_BLOCK);
    CHECK_FALSE(block->lang);

    TView<DocumentSpan*> spans = block->item.spans;
    CHECK(spans.size == 1);
    CHECK(spans.data[0]->type == DOCUMENT_SPAN_TEXT);
    CHECK(spans.data[0]->text == "indented\ncode\nblock via\ntabs\n");

    Document::destroy(doc);
    CHECK(get_memory_leaks(nullptr) == 0);
}

TEST_CASE("DocumentItem indented code block with spaces")
{
    Document doc = require_document("    indented\n    code\n    block via\n    four spaces\n");
    auto items = doc.get_items();
    CHECK(items.size == 1);

    DocumentItemCodeBlock* block = (DocumentItemCodeBlock*)items.data[0];
    CHECK(block->item.type == DOCUMENT_ITEM_CODE_BLOCK);
    CHECK_FALSE(block->lang);

    TView<DocumentSpan*> spans = block->item.spans;
    CHECK(spans.size == 1);
    CHECK(spans.data[0]->type == DOCUMENT_SPAN_TEXT);
    CHECK(spans.data[0]->text == "indented\ncode\nblock via\nfour spaces\n");

    Document::destroy(doc);
    CHECK(get_memory_leaks(nullptr) == 0);
}

TEST_CASE("DocumentItem unordered list")
{
    Document doc = require_document(R"(
- ul entry 1
- ul entry 2
- ul entry with `code` span
)");
    auto items = doc.get_items();
    CHECK(items.size == 3);

    auto* entry = (DocumentItemListEntry*)items.data[0];
    CHECK(entry->item.type == DOCUMENT_ITEM_LIST_ENTRY);
    CHECK(entry->index < 0);
    {
        TView<DocumentSpan*> spans = entry->item.spans;
        CHECK(spans.size == 1);
        CHECK(spans.data[0]->type == DOCUMENT_SPAN_TEXT);
        CHECK(spans.data[0]->text == "ul entry 1");
    }

    entry = (DocumentItemListEntry*)items.data[1];
    CHECK(entry->item.type == DOCUMENT_ITEM_LIST_ENTRY);
    CHECK(entry->index < 0);
    {
        TView<DocumentSpan*> spans = entry->item.spans;
        CHECK(spans.size == 1);
        CHECK(spans.data[0]->type == DOCUMENT_SPAN_TEXT);
        CHECK(spans.data[0]->text == "ul entry 2");
    }

    entry = (DocumentItemListEntry*)items.data[2];
    CHECK(entry->item.type == DOCUMENT_ITEM_LIST_ENTRY);
    CHECK(entry->index < 0);
    {
        TView<DocumentSpan*> spans = entry->item.spans;
        CHECK(spans.size == 3);
        CHECK(spans.data[0]->type == DOCUMENT_SPAN_TEXT);
        CHECK(spans.data[0]->text == "ul entry with ");
        CHECK(spans.data[1]->type == DOCUMENT_SPAN_CODE);
        CHECK(spans.data[1]->text == "code");
        CHECK(spans.data[2]->type == DOCUMENT_SPAN_TEXT);
        CHECK(spans.data[2]->text == " span");
    }

    Document::destroy(doc);
    CHECK(get_memory_leaks(nullptr) == 0);
}

TEST_CASE("DocumentItem ordered list")
{
    Document doc = require_document(R"(
67. ol entry 1
68. ol entry 2
69. ol entry with `code` span
)");
    auto items = doc.get_items();
    CHECK(items.size == 3);

    auto* entry = (DocumentItemListEntry*)items.data[0];
    CHECK(entry->item.type == DOCUMENT_ITEM_LIST_ENTRY);
    CHECK(entry->index == 67);
    {
        TView<DocumentSpan*> spans = entry->item.spans;
        CHECK(spans.size == 1);
        CHECK(spans.data[0]->type == DOCUMENT_SPAN_TEXT);
        CHECK(spans.data[0]->text == "ol entry 1");
    }

    entry = (DocumentItemListEntry*)items.data[1];
    CHECK(entry->item.type == DOCUMENT_ITEM_LIST_ENTRY);
    CHECK(entry->index == 68);
    {
        TView<DocumentSpan*> spans = entry->item.spans;
        CHECK(spans.size == 1);
        CHECK(spans.data[0]->type == DOCUMENT_SPAN_TEXT);
        CHECK(spans.data[0]->text == "ol entry 2");
    }

    entry = (DocumentItemListEntry*)items.data[2];
    CHECK(entry->item.type == DOCUMENT_ITEM_LIST_ENTRY);
    CHECK(entry->index == 69);
    {
        TView<DocumentSpan*> spans = entry->item.spans;
        CHECK(spans.size == 3);
        CHECK(spans.data[0]->type == DOCUMENT_SPAN_TEXT);
        CHECK(spans.data[0]->text == "ol entry with ");
        CHECK(spans.data[1]->type == DOCUMENT_SPAN_CODE);
        CHECK(spans.data[1]->text == "code");
        CHECK(spans.data[2]->type == DOCUMENT_SPAN_TEXT);
        CHECK(spans.data[2]->text == " span");
    }

    Document::destroy(doc);
    CHECK(get_memory_leaks(nullptr) == 0);
}
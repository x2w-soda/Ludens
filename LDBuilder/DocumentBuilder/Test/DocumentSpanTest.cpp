#include <Extra/doctest/doctest.h>
#include <Ludens/Memory/Memory.h>
#include <LudensBuilder/DocumentBuilder/Document.h>

using namespace LD;

TEST_CASE("DocumentSpan Link")
{
    static const char md[] = R"(a paragraph with [link text](doc://manual/path/to/file.md "link title") span)";
    Document doc = Document::create(View(md, sizeof(md) - 1));
    CHECK(doc);
    auto items = doc.get_items();
    CHECK(items.size == 1);

    DocumentItem* item = items.data[0];
    CHECK(item->type == DOCUMENT_ITEM_PARAGRAPH);
    CHECK(item->spans.size == 3);

    DocumentSpan* span = item->spans.data[0];
    CHECK(span->type == DOCUMENT_SPAN_TEXT);
    CHECK(span->text == "a paragraph with ");

    DocumentSpanLink* link = (DocumentSpanLink*)item->spans.data[1];
    CHECK(link->span.type == DOCUMENT_SPAN_LINK);
    CHECK(link->href == "doc://manual/path/to/file.md");
    CHECK(link->title == "link title");

    span = item->spans.data[2];
    CHECK(span->type == DOCUMENT_SPAN_TEXT);
    CHECK(span->text == " span");

    Document::destroy(doc);
    CHECK(get_memory_leaks(nullptr) == 0);
}

TEST_CASE("DocumentSpan Image")
{
    static const char md[] = R"(a paragraph with ![image](media://texture/path/to/image.png) span)";
    Document doc = Document::create(View(md, sizeof(md) - 1));
    CHECK(doc);
    auto items = doc.get_items();
    CHECK(items.size == 1);

    DocumentItem* item = items.data[0];
    CHECK(item->type == DOCUMENT_ITEM_PARAGRAPH);
    CHECK(item->spans.size == 3);

    DocumentSpan* span = item->spans.data[0];
    CHECK(span->type == DOCUMENT_SPAN_TEXT);
    CHECK(span->text == "a paragraph with ");

    DocumentSpanImage* img = (DocumentSpanImage*)item->spans.data[1];
    CHECK(img->span.type == DOCUMENT_SPAN_IMAGE);
    CHECK(img->uri == "media://texture/path/to/image.png");

    span = item->spans.data[2];
    CHECK(span->type == DOCUMENT_SPAN_TEXT);
    CHECK(span->text == " span");

    Document::destroy(doc);
    CHECK(get_memory_leaks(nullptr) == 0);
}
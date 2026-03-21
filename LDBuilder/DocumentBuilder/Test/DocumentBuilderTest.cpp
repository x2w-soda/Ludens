#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>
#include <LudensBuilder/DocumentBuilder/Document.h>

using namespace LD;

// early testing, needs some generalization later.
TEST_CASE("Document headings")
{
    static const char md[] = R"(
# heading 1
## heading 2
### heading 3
#### heading 4
##### heading 5
###### heading 6
)";
    Document doc = Document::create(View(md, sizeof(md) - 1));
    CHECK(doc);
    auto items = doc.get_items();
    CHECK(items.size == 6);

    for (int i = 1; i <= 6; i++)
    {
        DocumentItem* item = items.data[i - 1];
        CHECK(item->type == DOCUMENT_ITEM_HEADING);
        CHECK(item->spans.size == 1);

        std::string expected = std::format("heading {}", i);
        DocumentSpan* span = item->spans.data[0];
        CHECK(span->type == DOCUMENT_SPAN_TEXT);
        CHECK(span->text == expected.c_str());
    }

    Document::destroy(doc);

}
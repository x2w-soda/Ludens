#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>
#include <LudensBuilder/DocumentBuilder/Document.h>

using namespace LD;

// early testing, needs some generalization later.
TEST_CASE("Document")
{
    static const char md[] = R"(
# heading 1
## heading 2
)";
    Document doc = Document::create(View(md, sizeof(md) - 1));
    CHECK(doc);

    Document::destroy(doc);

}
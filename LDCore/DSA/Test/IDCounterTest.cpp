#include <Extra/doctest/doctest.h>
#include <Ludens/DSA/IDCounter.h>

using namespace LD;

TEST_CASE("IDCounter")
{
    IDCounter<uint8_t> ctr;

    uint8_t id = ctr.get_id();
    CHECK(id == 1);

    for (int i = 2; i <= 255; i++)
    {
        id = ctr.get_id();
        CHECK(id == i);
    }

    // must wrap around and skip zero
    id = ctr.get_id();
    CHECK(id == 1);

    id = ctr.get_id();
    CHECK(id == 2);
}

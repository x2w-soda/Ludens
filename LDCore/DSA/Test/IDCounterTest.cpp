#include <Extra/doctest/doctest.h>
#include <Ludens/DSA/IDCounter.h>

using namespace LD;

TEST_CASE("IDCounter")
{
    IDCounter<uint32_t> ctr;

    uint32_t id = ctr.get_id();
    CHECK(id == 1); // first valid ID.

    CHECK_FALSE(ctr.try_get_id(0));
    CHECK(ctr.try_get_id(UINT32_MAX)); // last valid ID for u32

    for (int i = 2; i <= 10; i++)
    {
        id = ctr.get_id();
        CHECK(id == i);
        CHECK_FALSE(ctr.try_get_id(id));
    }

    for (int i = 11; i <= 20; i++)
    {
        CHECK(ctr.try_get_id(i));
        CHECK_FALSE(ctr.try_get_id(i));
    }
}

TEST_CASE("IDCounter exhaust")
{
    IDCounter<uint8_t> ctr;

    for (int i = 2; i <= 254; i++)
        CHECK(ctr.try_get_id(i));

    CHECK(ctr.get_id() == 1);
    CHECK(ctr.get_id() == 255); // exhausted u8 ID space

    CHECK(ctr.get_id() == 0);
    CHECK(ctr.get_id() == 0);
}
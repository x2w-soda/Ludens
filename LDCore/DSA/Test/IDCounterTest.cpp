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

TEST_CASE("IDRegistry")
{
    IDRegistry<uint32_t> reg;

    uint32_t id = reg.get_id();
    CHECK(id == 1); // first valid ID.

    CHECK_FALSE(reg.try_get_id(0));
    CHECK(reg.try_get_id(UINT32_MAX)); // last valid ID for u32

    for (int i = 2; i <= 10; i++)
    {
        id = reg.get_id();
        CHECK(id == i);
        CHECK_FALSE(reg.try_get_id(id));
    }

    for (int i = 11; i <= 20; i++)
    {
        CHECK(reg.try_get_id(i));
        CHECK_FALSE(reg.try_get_id(i));
    }
}

TEST_CASE("IDRegistry exhaust")
{
    IDRegistry<uint8_t> reg;

    for (int i = 2; i <= 254; i++)
        CHECK(reg.try_get_id(i));

    CHECK(reg.get_id() == 1);
    CHECK(reg.get_id() == 255); // exhausted u8 ID space

    CHECK(reg.get_id() == 0);
    CHECK(reg.get_id() == 0);
}
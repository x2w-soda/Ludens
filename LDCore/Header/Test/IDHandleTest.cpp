#include <Extra/doctest/doctest.h>
#include <Ludens/Header/IDHandle.h>

#include <cstdint>

using namespace LD;

namespace {

struct Box
{
    int32_t value;
    uint32_t id;
};

} // namespace

TEST_CASE("IDHandle")
{
    Box b{1234, 3};
    IDHandle<Box, uint32_t> h1(&b, 3);

    CHECK(h1);
    Box* p = h1.unwrap();
    CHECK(p);
    CHECK(p->value == 1234);

    b.id = 4;
    CHECK(!h1); // invalid handle, should stop user from access
    CHECK(!h1.unwrap());

    b.id = 3;
    CHECK(!h1); // an invalid handle does not come back to life
    CHECK(!h1.unwrap());

    IDHandle<Box, uint32_t> h2{};
    CHECK(!h2);
    CHECK(!h2.unwrap());
}
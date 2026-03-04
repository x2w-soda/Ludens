#include <Extra/doctest/doctest.h>
#include <Ludens/DSA/IDRegistry.h>

using namespace LD;

TEST_CASE("IDReigstry")
{
    IDRegistry reg;

    ID id0 = reg.create();
    CHECK(id0);
    CHECK(id0.index() == 0);
    CHECK(id0.generation() == 1);

    ID id0gen1 = id0;
    reg.destroy(id0);
    id0 = reg.create();

    CHECK(id0);
    CHECK(id0.index() == 0); // biased reuse
    CHECK(id0.generation() == 2);
    CHECK(id0 != id0gen1);

    ID id1 = reg.create();
    CHECK(id1);
    CHECK(id1.index() == 1);
    CHECK(id1.generation() == 1);

    CHECK(id0 != id1);
}
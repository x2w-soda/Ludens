#include <Extra/doctest/doctest.h>
#include <Ludens/Header/Range.h>
#include <type_traits>

using namespace LD;

static_assert(std::is_trivial_v<Range>);

TEST_CASE("Range from_offsets")
{
    Range range;
    range = Range::from_offsets(0, 0);
    CHECK(range == Range(0, 0));

    range = Range::from_offsets(0, 4);
    CHECK(range == Range(0, 4));

    range = Range::from_offsets(4, 0);
    CHECK(range == Range(0, 4));

    range = Range::from_offsets(4, 9);
    CHECK(range == Range(4, 5));

    range = Range::from_offsets(9, 4);
    CHECK(range == Range(4, 5));
}

TEST_CASE("Range clamp_size")
{
    Range range;
    range = Range::clamp_size(Range(0, 0), 0);
    CHECK(range == Range(0, 0));

    range = Range::clamp_size(Range(4, 0), 0);
    CHECK(range == Range(0, 0));

    range = Range::clamp_size(Range(4, 0), 3);
    CHECK(range == Range(3, 0)); // clamp offset

    range = Range::clamp_size(Range(4, 5), 3);
    CHECK(range == Range(3, 0));

    range = Range::clamp_size(Range(4, 5), 9);
    CHECK(range == Range(4, 5));

    range = Range::clamp_size(Range(4, 5), 8);
    CHECK(range == Range(4, 4));

    range = Range::clamp_size(Range(4, 5), 5);
    CHECK(range == Range(4, 1));

    range = Range::clamp_size(Range(4, 5), 4);
    CHECK(range == Range(4, 0));
}
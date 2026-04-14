#include <Extra/doctest/doctest.h>
#include <Ludens/Header/Math/Rect.h>

using namespace LD;

TEST_CASE("Rect ctor")
{
    IRect r{};
    CHECK(r.x == 0);
    CHECK(r.y == 0);
    CHECK(r.w == 0);
    CHECK(r.h == 0);

    r = IRect(1, 2, 3, 4);
    CHECK(r.x == 1);
    CHECK(r.y == 2);
    CHECK(r.w == 3);
    CHECK(r.h == 4);
}

TEST_CASE("Rect method")
{
    IRect r(1, 2, 3, 4);
    CHECK(r.get_pos() == IVec2(1, 2));
    CHECK(r.get_size() == IVec2(3, 4));

    CHECK(!r.contains({1, 1}));
    CHECK(r.contains({1, 2}));
    CHECK(r.contains({2, 4}));
    CHECK(r.contains({4, 6}));
    CHECK(!r.contains({4, 7}));

    Rect r2(10, 20, 40, 60);
    Vec2 center = r2.get_center();
    CHECK(center == Vec2(30.0f, 50.0f));

    Vec2 p(0.0f, 10.0f);
    CHECK(is_equal_epsilon(r2.get_center_distance(p), 50.0f));

    float left, right, top, bot;
    r2.get_edge_distances(p, &left, &top, &right, &bot);
    CHECK(is_equal_epsilon(left, 10.0f));
    CHECK(is_equal_epsilon(top, 10.0f));
    CHECK(is_equal_epsilon(right, 50.0f));
    CHECK(is_equal_epsilon(bot, 70.0f));
}

TEST_CASE("Rect split")
{
    Rect area(10.0f, 10.0f, 100.0f, 100.0f);
    Rect tl, br, splitArea;
    Rect::split_v(0.25f, area, tl, br);
    CHECK(tl == Rect(10, 10, 25, 100));
    CHECK(br == Rect(35, 10, 75, 100));

    Rect::split_h(0.25f, area, tl, br);
    CHECK(tl == Rect(10, 10, 100, 25));
    CHECK(br == Rect(10, 35, 100, 75));

    Rect::split_v(0.25f, 10.0f, area, tl, br, splitArea);
    CHECK(tl == Rect(10, 10, 20, 100));
    CHECK(br == Rect(40, 10, 70, 100));
    CHECK(splitArea == Rect(30, 10, 10, 100));

    Rect::split_h(0.25f, 10.0f, area, tl, br, splitArea);
    CHECK(tl == Rect(10, 10, 100, 20));
    CHECK(br == Rect(10, 40, 100, 70));
    CHECK(splitArea == Rect(10, 30, 100, 10));
}

TEST_CASE("Rect scale")
{
    Rect area(10.0f, 10.0f, 100.0f, 100.0f);

    Rect scaled = Rect::scale_h(area, 0.0f);
    CHECK(scaled == area);
    scaled = Rect::scale_h(area, -0.1f);
    CHECK(scaled == area);

    scaled = Rect::scale_h(area, 0.5f);
    CHECK(scaled == Rect(10.0f, 35.0f, 100.0f, 50.0f));

    scaled = Rect::scale_h(area, 2.0f);
    CHECK(scaled == Rect(10.0f, -40.0f, 100.0f, 200.0f));

    scaled = Rect::scale_w(area, 0.0f);
    CHECK(scaled == area);
    scaled = Rect::scale_w(area, -0.1f);
    CHECK(scaled == area);

    scaled = Rect::scale_w(area, 0.5f);
    CHECK(scaled == Rect(35.0f, 10.0f, 50.0f, 100.0f));

    scaled = Rect::scale_w(area, 2.0f);
    CHECK(scaled == Rect(-40.0f, 10.0f, 200.0f, 100.0f));
}

TEST_CASE("Rect rotate")
{
    Rect area(-1.0f, -0.5f, 2.0f, 1.0f);
    Vec2 center = area.get_center();
    Rect rotated = Rect::rotate(area, 0.0f);
    CHECK(rotated == area);

    rotated = Rect::rotate(area, LD_TO_RADIANS(360.0f));
    CHECK(rotated == area);

    rotated = Rect::rotate(area, LD_TO_RADIANS(90.0f));
    CHECK(rotated == Rect(center.x - 0.5f, center.y - 1.0f, 1.0f, 2.0f));
}

TEST_CASE("Rect map_normalized")
{
    Rect region(-10.0f, -20.0f, 100.0f, 200.0f);
    Rect rect;

    rect = Rect::map_normalized(region, Rect(0.0f, 0.0f, 1.0f, 1.0f));
    CHECK(rect == region);

    rect = Rect::map_normalized(region, Rect(0.0f, 0.0f, 0.5f, 0.25f));
    CHECK(rect == Rect(-10.0f, -20.0f, 50.0f, 50.0f));

    rect = Rect::map_normalized(region, Rect(0.2f, 0.4f, 0.5f, 0.25f));
    CHECK(rect == Rect(10.0f, 60.0f, 50.0f, 50.0f));
}

TEST_CASE("Rect get_union")
{
    Rect rect;
    
    rect = Rect::get_union(Rect(-10.0f, -20.0f, 100.0f, 200.0f), Rect());
    CHECK(rect == Rect(-10.0f, -20.0f, 100.0f, 200.0f));

    rect = Rect::get_union(Rect(-10.0f, -20.0f, 100.0f, 200.0f), Rect(50.0f, 50.0f, 100.0f, 100.0f));
    CHECK(rect == Rect(-10.0f, -20.0f, 160.0f, 200.0f));
}
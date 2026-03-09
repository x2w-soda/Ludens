#include <Extra/doctest/doctest.h>
#include <Ludens/Camera/Camera2D.h>
#include <Ludens/Memory/Memory.h>

using namespace LD;

TEST_CASE("Camera2D")
{
    Camera2D camera = Camera2D::create(Vec2(100.0f, 100.0f));

    Vec2 pos = camera.get_position();
    CHECK(pos == Vec2(50.0f, 50.0f));

    CHECK(camera.get_zoom() == 1.0f);
    CHECK(camera.get_rotation() == 0.0f);
    CHECK(camera.get_world_aabb() == Rect(0.0f, 0.0f, 100.0f, 100.0f));

    Camera2D::destroy(camera);

	CHECK_FALSE(get_memory_leaks(nullptr));
}

TEST_CASE("Camera2D World AABB")
{
    constexpr float width = 200.0f;
    constexpr float height = 100.0f;
    constexpr float halfW = width / 2.0f;
    constexpr float halfH = height / 2.0f;
    const Vec2 center(halfW, halfH);
    Camera2D camera = Camera2D::create(Vec2(width, height));
    REQUIRE(camera.get_zoom() == 1.0f);
    REQUIRE(camera.get_rotation() == 0.0f);
    REQUIRE(camera.get_position() == center);

    CHECK(camera.get_world_aabb() == Rect(0.0f, 0.0f, width, height));

    // resistant to zoom
    camera.set_zoom(0.5f);
    float worldW = width * 2.0f;
    float worldH = height * 2.0f;
    CHECK(camera.get_world_aabb() == Rect(center.x - worldW / 2.0f, center.y - worldH / 2.0f, worldW, worldH));

    camera.set_zoom(2.0f);
    worldW = width * 0.5f;
    worldH = height * 0.5f;
    CHECK(camera.get_world_aabb() == Rect(center.x - worldW / 2.0f, center.y - worldH / 2.0f, worldW, worldH));

    // TODO: rotation

    Camera2D::destroy(camera);
    CHECK_FALSE(get_memory_leaks(nullptr));
}
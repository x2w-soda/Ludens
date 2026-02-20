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

    Camera2D::destroy(camera);

	CHECK_FALSE(get_memory_leaks(nullptr));
}
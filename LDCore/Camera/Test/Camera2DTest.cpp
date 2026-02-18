#include <Extra/doctest/doctest.h>
#include <Ludens/Camera/Camera2D.h>
#include <Ludens/Memory/Memory.h>

using namespace LD;

TEST_CASE("Camera2D")
{
    Camera2DInfo info = Camera2DInfo::extent(100.0f, 100.0f);
    Camera2D camera = Camera2D::create(info);

    Vec2 pos = camera.get_position();
    CHECK(pos == Vec2(0.0f));

    float rot = camera.get_rotation();
    CHECK(is_equal_epsilon(rot, 0.0f));

    Camera2D::destroy(camera);

	CHECK_FALSE(get_memory_leaks(nullptr));
}
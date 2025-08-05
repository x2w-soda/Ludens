#pragma once

#include <Extra/doctest/doctest.h>
#include <Ludens/Camera/Camera.h>
#include <Ludens/Header/Math/Math.h>

using namespace LD;

TEST_CASE("Perspective screen_to_world_size")
{
    const float screenW = 1600.0f;
    const float screenH = 900.0f;
    CameraPerspectiveInfo camI{};
    camI.aspectRatio = screenW / screenH;
    camI.fov = LD_PI_2; // 90 degrees
    camI.nearClip = 0.01f;
    camI.farClip = 100.0f;
    Camera cam = Camera::create(camI, Vec3(0.0f));
    cam.set_pos(Vec3(0.0f, 0.0f, 10.0f));

    // view depth 10, desire half screen height
    float worldSize = cam.screen_to_world_size(Vec3(0.0f), screenH, 450.0f);
    CHECK(is_equal_epsilon(worldSize, 10.0f));

    // view depth 10, desire full screen height
    worldSize = cam.screen_to_world_size(Vec3(0.0f), screenH, 900.0f);
    CHECK(is_equal_epsilon(worldSize, 20.0f));

    // view depth 100, desire half screen height
    worldSize = cam.screen_to_world_size(Vec3(0.0f, 0.0f, -90.0f), screenH, 450.0f);
    CHECK(is_equal_epsilon(worldSize, 100.0f));

    // view depth 100, camera not aligned, desire half screen height
    worldSize = cam.screen_to_world_size(Vec3(123.0f, 456.0f, -90.0f), screenH, 450.0f);
    CHECK(is_equal_epsilon(worldSize, 100.0f));

    // TODO: worldSize can be negative due to dot product

    Camera::destroy(cam);
}

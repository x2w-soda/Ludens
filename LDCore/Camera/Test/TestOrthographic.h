#pragma once

#include <Extra/doctest/doctest.h>
#include <Ludens/Camera/Camera.h>
#include <Ludens/Header/Math/Math.h>

using namespace LD;

TEST_CASE("Orthographic screen_to_world_size")
{
    const float screenH = 900.0f;
    const float screenW = 1600.0f;
    CameraOrthographicInfo camI{};
    camI.top = 0.0f;
    camI.left = 0.0f;
    camI.bottom = screenH;
    camI.right = screenW;
    camI.nearClip = 1.0f;
    camI.farClip = 100.0f;
    Camera cam = Camera::create(camI, Vec3(0.0f));
    cam.set_pos(Vec3(0.0f, 0.0f, 10.0f));

    // desire half screen height
    float worldSize = cam.screen_to_world_size(Vec3(0.0f), screenH, 450.0f);
    CHECK(is_equal_epsilon(worldSize, 450.0f));

    // view position and view depth does not matter in orthographic
    cam.set_pos(Vec3(123.0f, 456.0f, 789.0f));

    // desire one-third of screen height
    worldSize = cam.screen_to_world_size(Vec3(987.0f, 654.0f, 321.0f), screenH, 300.0f);
    CHECK(is_equal_epsilon(worldSize, 300.0f));

    // screen height is half of frustum height
    worldSize = cam.screen_to_world_size(Vec3(987.0f, 654.0f, 321.0f), screenH / 2.0f, 300.0f);
    CHECK(is_equal_epsilon(worldSize, 600.0f));
}
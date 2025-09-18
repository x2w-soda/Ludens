#pragma once

#include <Extra/doctest/doctest.h>
#include <Ludens/Header/Color.h>

using namespace LD;

TEST_CASE("Color")
{
    Color c1{};
    CHECK((uint32_t)c1 == 0);

    c1 = Vec4(1.0f, 0.0, 0.0f, 0.0f);
    CHECK((uint32_t)c1 == 0xFF000000);

    c1 = Vec4(0.0f, 1.0, 0.0f, 0.0f);
    CHECK((uint32_t)c1 == 0x00FF0000);

    c1 = Vec4(0.0f, 0.0, 1.0f, 0.0f);
    CHECK((uint32_t)c1 == 0x0000FF00);

    c1 = Vec4(0.0f, 0.0, 0.0f, 1.0f);
    CHECK((uint32_t)c1 == 0x000000FF);

    c1 = Vec4(1.0f);
    CHECK((uint32_t)c1 == 0xFFFFFFFF);

    c1 = Vec3(1.0f, 0.0f, 0.0f);
    CHECK((uint32_t)c1 == 0xFF0000FF);

    c1 = Vec3(0.0f, 1.0f, 0.0f);
    CHECK((uint32_t)c1 == 0x00FF00FF);

    c1 = Vec3(0.0f, 0.0f, 1.0f);
    CHECK((uint32_t)c1 == 0x0000FFFF);

    c1 = Vec3(1.0f);
    CHECK((uint32_t)c1 == 0xFFFFFFFF);

    c1 = 0xCAFEBABE;
    CHECK((uint32_t)c1 == 0xCAFEBABE);

    c1.set_alpha(0.2f);
    CHECK((uint32_t)c1 == 0xCAFEBA33);
}
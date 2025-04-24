#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Header/Math/Vec3.h>
#include <Ludens/Header/Math/Vec4.h>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>

using namespace LD;

TEST_CASE("Vec2 ctor")
{
    IVec2 v;
    CHECK(v.x == 0);
    CHECK(v.y == 0);

    v = IVec2(3);
    CHECK(v.x == 3);
    CHECK(v.y == 3);

    v = IVec2(1, 2);
    CHECK(v.x == 1);
    CHECK(v.y == 2);

    IVec2 v2 = v;
    CHECK(v2.x == 1);
    CHECK(v2.y == 2);
}

TEST_CASE("Vec2 member")
{
    IVec2 v(2, 3);
    CHECK(v.r == 2);
    CHECK(v.g == 3);
    v.r = 4;
    v.g = 5;
    CHECK(v.x == 4);
    CHECK(v.y == 5);
}

TEST_CASE("Vec2 method")
{
    IVec2 v1(1, 2);
    IVec2 v2(2, -3);

    CHECK(v2.length_squared() == 13);

    CHECK(IVec2::dot(v1, v2) == -4);
    CHECK(IVec2::dot(v2, v1) == -4);

    unsigned long long ullData[2] = {2, 3};
    double f64Data[2] = {4.0, 5.0};
    v1 = IVec2::from_data(ullData);
    v2 = IVec2::from_data(f64Data);
    CHECK(v1.x == 2);
    CHECK(v1.y == 3);
    CHECK(v2.x == 4);
    CHECK(v2.y == 5);
}

TEST_CASE("Vec3 ctor")
{
    IVec3 v;
    CHECK(v.x == 0);
    CHECK(v.y == 0);
    CHECK(v.z == 0);

    v = IVec3(2);
    CHECK(v.x == 2);
    CHECK(v.y == 2);
    CHECK(v.z == 2);

    v = IVec3(1, 2, 3);
    CHECK(v.x == 1);
    CHECK(v.y == 2);
    CHECK(v.z == 3);

    IVec3 v2 = v;
    CHECK(v2.x == 1);
    CHECK(v2.y == 2);
    CHECK(v2.z == 3);
}

TEST_CASE("Vec3 member")
{
    IVec3 v(2, 3, 4);
    CHECK(v.r == 2);
    CHECK(v.g == 3);
    CHECK(v.b == 4);
    v.r = 5;
    v.g = 6;
    v.b = 7;
    CHECK(v.x == 5);
    CHECK(v.y == 6);
    CHECK(v.z == 7);
}

TEST_CASE("Vec3 method")
{
    IVec3 v1(1, 2, 3);
    IVec3 v2(2, -3, 4);

    CHECK(v2.length_squared() == 29);

    CHECK(IVec3::dot(v1, v2) == 8);
    CHECK(IVec3::dot(v2, v1) == 8);

    unsigned long long ullData[3] = {1, 2, 3};
    double f64Data[3] = {4.0, 5.0, 6.0};
    v1 = IVec3::from_data(ullData);
    v2 = IVec3::from_data(f64Data);
    CHECK(v1.x == 1);
    CHECK(v1.y == 2);
    CHECK(v1.z == 3);
    CHECK(v2.x == 4);
    CHECK(v2.y == 5);
    CHECK(v2.z == 6);
}

TEST_CASE("Vec4 ctor")
{
    IVec4 v;
    CHECK(v.x == 0);
    CHECK(v.y == 0);
    CHECK(v.z == 0);
    CHECK(v.w == 0);

    v = IVec4(2);
    CHECK(v.x == 2);
    CHECK(v.y == 2);
    CHECK(v.z == 2);
    CHECK(v.w == 2);

    v = IVec4(1, 2, 3, 4);
    CHECK(v.x == 1);
    CHECK(v.y == 2);
    CHECK(v.z == 3);
    CHECK(v.w == 4);

    IVec4 v2 = v;
    CHECK(v2.x == 1);
    CHECK(v2.y == 2);
    CHECK(v2.z == 3);
    CHECK(v2.w == 4);

    v2 = IVec4(IVec2(1, 2), IVec2(3, 4));
    CHECK(v2.x == 1);
    CHECK(v2.y == 2);
    CHECK(v2.z == 3);
    CHECK(v2.w == 4);

    v2 = IVec4(IVec3(1, 2, 3), 4);
    CHECK(v2.x == 1);
    CHECK(v2.y == 2);
    CHECK(v2.z == 3);
    CHECK(v2.w == 4);

    v2 = IVec4(1, IVec3(2, 3, 4));
    CHECK(v2.x == 1);
    CHECK(v2.y == 2);
    CHECK(v2.z == 3);
    CHECK(v2.w == 4);
}

TEST_CASE("Vec4 member")
{
    IVec4 v(1, 2, 3, 4);
    CHECK(v.r == 1);
    CHECK(v.g == 2);
    CHECK(v.b == 3);
    CHECK(v.a == 4);
    v.r = 5;
    v.g = 6;
    v.b = 7;
    v.a = 8;
    CHECK(v.x == 5);
    CHECK(v.y == 6);
    CHECK(v.z == 7);
    CHECK(v.w == 8);
}

TEST_CASE("Vec4 method")
{
    IVec4 v1(1, 2, 3, 4);
    IVec4 v2(2, -3, 4, 5);

    CHECK(v2.length_squared() == 54);

    CHECK(IVec4::dot(v1, v2) == 28);
    CHECK(IVec4::dot(v2, v1) == 28);

    unsigned long long ullData[4] = {1, 2, 3, 4};
    double f64Data[4] = {4.0, 5.0, 6.0, 7.0};
    v1 = IVec4::from_data(ullData);
    v2 = IVec4::from_data(f64Data);
    CHECK(v1.x == 1);
    CHECK(v1.y == 2);
    CHECK(v1.z == 3);
    CHECK(v1.w == 4);
    CHECK(v2.x == 4);
    CHECK(v2.y == 5);
    CHECK(v2.z == 6);
    CHECK(v2.w == 7);
}
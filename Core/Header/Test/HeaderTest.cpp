#include <Ludens/Header/Math/Mat3.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/Header/Math/Quat.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Header/Math/Vec3.h>
#include <Ludens/Header/Math/Vec4.h>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>
#include "TestHash.h"

using namespace LD;

static_assert(sizeof(Vec2) == 8);
static_assert(sizeof(IVec2) == 8);
static_assert(sizeof(DVec2) == 16);

static_assert(sizeof(Vec3) == 12);
static_assert(sizeof(IVec3) == 12);
static_assert(sizeof(DVec3) == 24);

static_assert(sizeof(Vec4) == 16);
static_assert(sizeof(IVec4) == 16);
static_assert(sizeof(DVec4) == 32);

TEST_CASE("Math")
{
    CHECK(is_zero_epsilon<float>(LD_EPSILON_F32 / +2.0f));
    CHECK(is_zero_epsilon<float>(LD_EPSILON_F32 / -2.0f));
    CHECK(is_zero_epsilon<float>(LD_EPSILON_F64 / +2.0));
    CHECK(is_zero_epsilon<float>(LD_EPSILON_F64 / -2.0));
    CHECK(!is_zero_epsilon<double>(LD_EPSILON_F32 / +2.0f));
    CHECK(!is_zero_epsilon<double>(LD_EPSILON_F32 / -2.0f));
    CHECK(is_zero_epsilon<double>(LD_EPSILON_F64 / +2.0f));
    CHECK(is_zero_epsilon<double>(LD_EPSILON_F64 / -2.0f));

    CHECK(is_zero_epsilon<int>(0));
    CHECK(!is_zero_epsilon<int>(1));
    CHECK(!is_zero_epsilon<float>(0.0001f));
    CHECK(!is_zero_epsilon<double>(0.0000001));
}

TEST_CASE("Vec2 ctor")
{
    IVec2 v{};
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
    IVec2 v3(7, -24);

    CHECK(v2.length_squared() == 13);
    CHECK(v3.length_squared() == 625);
    CHECK(v3.length() == 25);
    CHECK(IVec2(0).length() == 0);
    CHECK(IVec2(0).length_squared() == 0);

    CHECK(IVec2::dot(v1, v2) == -4);
    CHECK(IVec2::dot(v2, v1) == -4);

    CHECK(Vec2::normalize(Vec2(7.0f, -24.0f)) == Vec2(0.28f, -0.96f));
    CHECK(Vec2::normalize(Vec2(-3.0f, 0.0f)) == Vec2(-1.0f, 0.0f));
    CHECK(Vec2::normalize(Vec2(3.0f, -4.0f)) == Vec2(0.6f, -0.8f));

    unsigned long long ullData[2] = {2, 3};
    double f64Data[2] = {4.0, 5.0};
    v1 = IVec2::from_data(ullData);
    v2 = IVec2::from_data(f64Data);
    CHECK(v1.x == 2);
    CHECK(v1.y == 3);
    CHECK(v2.x == 4);
    CHECK(v2.y == 5);
}

TEST_CASE("Vec2 operator")
{
    IVec2 v1(1, 2);
    IVec2 v2(3, -4);

    CHECK(v1 == IVec2(1, 2));
    CHECK(v1 != v2);

    CHECK(v1 + v2 == IVec2(4, -2));
    CHECK(v1 - v2 == IVec2(-2, 6));
    CHECK(v1 * v2 == IVec2(3, -8));
    CHECK(v1 / v2 == IVec2(0, 0));

    CHECK(v1 + 3 == IVec2(4, 5));
    CHECK(v1 - 3 == IVec2(-2, -1));
    CHECK(v1 * 3 == IVec2(3, 6));
    CHECK(v1 / 3 == IVec2(0, 0));

    v1 += IVec2(3, 4);
    CHECK(v1 == IVec2(4, 6));
    v1 -= IVec2(2, 2);
    CHECK(v1 == IVec2(2, 4));
    v1 *= IVec2(3);
    CHECK(v1 == IVec2(6, 12));
    v1 /= IVec2(2);
    CHECK(v1 == IVec2(3, 6));

    CHECK(+v1 == v1);
    CHECK(-v1 == v1 * -1);
}

TEST_CASE("Vec3 ctor")
{
    IVec3 v{};
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

    v = IVec3(IVec2(1, 2), 3);
    CHECK(v.x == 1);
    CHECK(v.y == 2);
    CHECK(v.z == 3);

    v = IVec3(1, IVec2(2, 3));
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

TEST_CASE("Vec3 operator")
{
    IVec3 v1(1, 2, 3);
    IVec3 v2(5, -6, 7);

    CHECK(v1 == IVec3(1, 2, 3));
    CHECK(v1 != v2);

    CHECK(v1 + v2 == IVec3(6, -4, 10));
    CHECK(v1 - v2 == IVec3(-4, 8, -4));
    CHECK(v1 * v2 == IVec3(5, -12, 21));
    CHECK(v1 / v2 == IVec3(0, 0, 0));

    CHECK(v1 + 3 == IVec3(4, 5, 6));
    CHECK(v1 - 3 == IVec3(-2, -1, 0));
    CHECK(v1 * 3 == IVec3(3, 6, 9));
    CHECK(v1 / 3 == IVec3(0, 0, 1));

    v1 += IVec3(3, 4, 5);
    CHECK(v1 == IVec3(4, 6, 8));
    v1 -= IVec3(2, 2, 2);
    CHECK(v1 == IVec3(2, 4, 6));
    v1 *= IVec3(3);
    CHECK(v1 == IVec3(6, 12, 18));
    v1 /= IVec3(2);
    CHECK(v1 == IVec3(3, 6, 9));

    CHECK(+v1 == v1);
    CHECK(-v1 == v1 * -1);
}

TEST_CASE("Vec3 method")
{
    IVec3 v1(1, 2, 3);
    IVec3 v2(2, -3, 4);
    IVec3 v3(3, -4, 0);

    CHECK(v2.length_squared() == 29);
    CHECK(v3.length() == 5);
    CHECK(IVec3(0).length() == 0);
    CHECK(IVec3(0).length_squared() == 0);

    CHECK(IVec3::dot(v1, v2) == 8);
    CHECK(IVec3::dot(v2, v1) == 8);

    CHECK(IVec3::cross(IVec3(1, 0, 0), IVec3(0, 1, 0)) == IVec3(0, 0, 1));
    CHECK(IVec3::cross(IVec3(0, 1, 0), IVec3(1, 0, 0)) == IVec3(0, 0, -1));
    CHECK(IVec3::cross(v1, v2) == IVec3(17, 2, -7));

    CHECK(Vec3::normalize(Vec3(-3.0f, 0.0f, 0.0f)) == Vec3(-1.0f, 0.0f, 0.0f));
    CHECK(Vec3::normalize(Vec3(7.0f, -24.0f, 0.0f)) == Vec3(0.28f, -0.96f, 0.0f));
    CHECK(Vec3::normalize(Vec3(0.0f, 3.0f, -4.0f)) == Vec3(0.0f, 0.6f, -0.8f));

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
    IVec4 v{};
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

TEST_CASE("Vec4 operator")
{
    IVec4 v1(1, 2, 3, 4);
    IVec4 v2(5, -6, 7, 8);

    CHECK(v1 == IVec4(1, 2, 3, 4));
    CHECK(v1 != v2);

    CHECK(v1 + v2 == IVec4(6, -4, 10, 12));
    CHECK(v1 - v2 == IVec4(-4, 8, -4, -4));
    CHECK(v1 * v2 == IVec4(5, -12, 21, 32));
    CHECK(v1 / v2 == IVec4(0, 0, 0, 0));

    CHECK(v1 + 3 == IVec4(4, 5, 6, 7));
    CHECK(v1 - 3 == IVec4(-2, -1, 0, 1));
    CHECK(v1 * 3 == IVec4(3, 6, 9, 12));
    CHECK(v1 / 3 == IVec4(0, 0, 1, 1));

    v1 += IVec4(3, 4, 5, 6);
    CHECK(v1 == IVec4(4, 6, 8, 10));
    v1 -= IVec4(2);
    CHECK(v1 == IVec4(2, 4, 6, 8));
    v1 *= IVec4(3);
    CHECK(v1 == IVec4(6, 12, 18, 24));
    v1 /= IVec4(2);
    CHECK(v1 == IVec4(3, 6, 9, 12));

    CHECK(+v1 == v1);
    CHECK(-v1 == v1 * -1);
}

TEST_CASE("Vec4 method")
{
    IVec4 v1(1, 2, 3, 4);
    IVec4 v2(2, -3, 4, 5);
    IVec4 v3(10, -4, 2, -1);
    Vec4 v4(9, 3, -3, 1);

    CHECK(v3.as_vec3() == IVec3(10, -4, 2));

    CHECK(v1.length_squared() == 30);
    CHECK(v2.length_squared() == 54);
    CHECK(v3.length_squared() == 121);
    CHECK(v3.length() == 11);
    CHECK(IVec4(0).length() == 0);
    CHECK(IVec4(0).length_squared() == 0);

    CHECK(IVec4::dot(v1, v2) == 28);
    CHECK(IVec4::dot(v2, v1) == 28);

    CHECK(Vec4::normalize(v4) == Vec4(0.9f, 0.3f, -0.3f, 0.1f));

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

TEST_CASE("Quat ctor")
{
    Quat q;
    CHECK(q.x == 0.0f);
    CHECK(q.y == 0.0f);
    CHECK(q.z == 0.0f);
    CHECK(q.w == 1.0f);

    q = Quat(1, 2, 3, 4);
    CHECK(q.x == 1.0f);
    CHECK(q.y == 2.0f);
    CHECK(q.z == 3.0f);
    CHECK(q.w == 4.0f);
}

TEST_CASE("Quat method")
{
    int iData[4] = {1, 2, 3, 4};

    // memory order is X, Y, Z, W
    Quat q = Quat::from_data(iData);
    CHECK(q.x == 1.0f);
    CHECK(q.y == 2.0f);
    CHECK(q.z == 3.0f);
    CHECK(q.w == 4.0f);
}

TEST_CASE("Rect ctor")
{
    IRect r;
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
}

TEST_CASE("Mat3 ctor")
{
    IMat3 m;
    CHECK(m[0] == IVec3(0));
    CHECK(m[1] == IVec3(0));
    CHECK(m[2] == IVec3(0));

    m = IMat3(IVec3(1), IVec3(2), IVec3(3));
    CHECK(m[0] == IVec3(1));
    CHECK(m[1] == IVec3(2));
    CHECK(m[2] == IVec3(3));

    m = IMat3(4);
    CHECK(m[0] == IVec3(4, 0, 0));
    CHECK(m[1] == IVec3(0, 4, 0));
    CHECK(m[2] == IVec3(0, 0, 4));
}

TEST_CASE("Mat3 operator")
{
    IVec3 v1(1, 2, 3);

    IMat3 m = IMat3(2) * IMat3(4);
    CHECK(m[0] == IVec3(8, 0, 0));
    CHECK(m[1] == IVec3(0, 8, 0));
    CHECK(m[2] == IVec3(0, 0, 8));

    IVec3 v2 = m * v1;
    CHECK(v2 == IVec3(8, 16, 24));
}

TEST_CASE("Mat3 method")
{
    Mat3 m({1, 0, 0}, {2, 1, 0}, {3, 2, 1});
    Mat3 m2 = Mat3::transpose(m);
    CHECK(m2[0] == Vec3(1, 2, 3));
    CHECK(m2[1] == Vec3(0, 1, 2));
    CHECK(m2[2] == Vec3(0, 0, 1));

    Vec3 p1(2, -2, 3);

    Mat3 mi = Mat3::inverse(m);
    Vec3 p2 = mi * m * p1;
    CHECK(p2 == p1);

    p2 = m * mi * p1;
    CHECK(p2 == p1);
}

TEST_CASE("Mat4 ctor")
{
    IMat4 m;
    CHECK(m[0] == IVec4(0, 0, 0, 0));
    CHECK(m[1] == IVec4(0, 0, 0, 0));
    CHECK(m[2] == IVec4(0, 0, 0, 0));
    CHECK(m[3] == IVec4(0, 0, 0, 0));

    m = IMat4(IVec4(1), IVec4(2), IVec4(3), IVec4(4));
    CHECK(m[0] == IVec4(1, 1, 1, 1));
    CHECK(m[1] == IVec4(2, 2, 2, 2));
    CHECK(m[2] == IVec4(3, 3, 3, 3));
    CHECK(m[3] == IVec4(4, 4, 4, 4));

    m = IMat4(3);
    CHECK(m[0] == IVec4(3, 0, 0, 0));
    CHECK(m[1] == IVec4(0, 3, 0, 0));
    CHECK(m[2] == IVec4(0, 0, 3, 0));
    CHECK(m[3] == IVec4(0, 0, 0, 3));
}

TEST_CASE("Mat4 operator")
{
    IVec4 v1(1, 2, 3, 4);

    IMat4 m = IMat4(2) * IMat4(3);
    CHECK(m[0] == IVec4(6, 0, 0, 0));
    CHECK(m[1] == IVec4(0, 6, 0, 0));
    CHECK(m[2] == IVec4(0, 0, 6, 0));
    CHECK(m[3] == IVec4(0, 0, 0, 6));

    IVec4 v2 = m * v1;
    CHECK(v2 == IVec4(6, 12, 18, 24));
}

TEST_CASE("Mat4 method")
{
    Vec4 p1(3, 2, 4, 1);

    Mat4 m(p1, p1 * 2.0f, p1 * 3.0f, p1 * 4.0f);
    Mat3 m2 = m.as_mat3();
    CHECK(m2[0] == Vec3(3, 2, 4));
    CHECK(m2[1] == Vec3(6, 4, 8));
    CHECK(m2[2] == Vec3(9, 6, 12));

    Mat4 rot = Mat4::from_quat({});
    CHECK(rot * p1 == p1);

    Vec4 p2 = Mat4::translate({2, -1, 3}) * p1;
    CHECK(p2 == Vec4(5, 1, 7, 1));

    p2 = Mat4::scale({3, -2, 4}) * p1;
    CHECK(p2 == Vec4(9, -4, 16, 1));

    Vec4 p3 = Mat4::rotate(LD_PI_2, Vec3(0, 0, 1)) * Vec4(2, 10, 7, 1);
    CHECK(p3 == Vec4(-10, 2, 7, 1));
}
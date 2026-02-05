#include <Ludens/Header/Bitwise.h>
#include <Ludens/Header/Impulse.h>
#include <Ludens/Header/Math/Mat3.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/Header/Math/Quat.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Header/Math/Vec3.h>
#include <Ludens/Header/Math/Vec4.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>

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

TEST_CASE("Bitwise")
{
    CHECK(next_power_of_two(0) == 0);
    CHECK(next_power_of_two(1) == 1);
    CHECK(next_power_of_two(2) == 2);
    CHECK(next_power_of_two(3) == 4);
    CHECK(next_power_of_two(7) == 8);
    CHECK(next_power_of_two(8) == 8);
    CHECK(next_power_of_two(33) == 64);
    CHECK(next_power_of_two(16384) == 16384);
    CHECK(next_power_of_two(16385) == 32768);
    CHECK(next_power_of_two(2147483648) == 2147483648);
}

TEST_CASE("Impulse")
{
    Impulse flag;

    flag.set(false);
    CHECK(!flag.read());

    flag.set(true);
    CHECK(flag.read());
    CHECK(!flag.read()); // reset after a successful read

    TImpulse<int> iflag;

    iflag.set(0);
    CHECK(!iflag.read());

    iflag.set(30);
    CHECK(iflag.read() == 30);
    CHECK(iflag.read() == 0); // reset after a successful read
}

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

    CHECK(v[0] == 5);
    CHECK(v[1] == 6);
    CHECK(v[2] == 7);

    v[0] = 8;
    v[1] = 9;
    v[2] = 10;

    CHECK(v.r == 8);
    CHECK(v.g == 9);
    CHECK(v.b == 10);
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

    CHECK(v[0] == 5);
    CHECK(v[1] == 6);
    CHECK(v[2] == 7);
    CHECK(v[3] == 8);

    v[0] = 9;
    v[1] = 10;
    v[2] = 11;
    v[3] = 12;

    CHECK(v.r == 9);
    CHECK(v.g == 10);
    CHECK(v.b == 11);
    CHECK(v.a == 12);
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
    Quat q = Quat(1, 2, 3, 4);
    CHECK(q.x == 1.0f);
    CHECK(q.y == 2.0f);
    CHECK(q.z == 3.0f);
    CHECK(q.w == 4.0f);

    q = Quat(Vec3(1, 2, 3), 4);
    CHECK(q.x == 1.0f);
    CHECK(q.y == 2.0f);
    CHECK(q.z == 3.0f);
    CHECK(q.w == 4.0f);

    CHECK(q[0] == 1.0f);
    CHECK(q[1] == 2.0f);
    CHECK(q[2] == 3.0f);
    CHECK(q[3] == 4.0f);

    q[0] = 5.0f;
    q[1] = 6.0f;
    q[2] = 7.0f;
    q[3] = 8.0f;

    CHECK(q.x == 5.0f);
    CHECK(q.y == 6.0f);
    CHECK(q.z == 7.0f);
    CHECK(q.w == 8.0f);
}

TEST_CASE("Quat operator")
{
    Quat q = Quat::from_axis_angle(Vec3(0.0f, 1.0f, 0.0f), LD_PI_2);
    Vec3 p1(1.0f, -3.0f, 0.0f);
    Vec3 p2 = q * p1;
    CHECK(p2 == Vec3(0.0f, -3.0f, -1.0f));
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

    q = Quat(0, 0, 0, 3);
    CHECK(is_equal_epsilon(q.length(), 3.0f));
    q = Quat::normalize(q);
    CHECK(q.is_normalized());
    CHECK(is_equal_epsilon(q.length(), 1.0f));

    Mat3 rot = Mat3::rotate_x(90);
    q = Quat::from_mat3(rot);
    Vec3 p1 = Vec3(1, 2, 3);
    Vec3 p2 = q * p1;
    Vec3 p3 = rot * p1;
    CHECK(p2 == p3);
    Vec3 e = q.as_euler();
    CHECK(e == Vec3(90, 0, 0));

    rot = Mat3::rotate_y(90);
    q = Quat::from_mat3(rot);
    p2 = q * p1;
    p3 = rot * p1;
    CHECK(p2 == p3);
    e = q.as_euler();
    CHECK(e == Vec3(0, 90, 0));

    rot = Mat3::rotate_z(-90);
    q = Quat::from_mat3(rot);
    p2 = q * p1;
    p3 = rot * p1;
    CHECK(p2 == p3);
    e = q.as_euler();
    CHECK(e == Vec3(0, 0, -90));
}

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

TEST_CASE("Mat3 ctor")
{
    IMat3 m{};
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
    CHECK(m.element(0) == 1);
    CHECK(m.element(1) == 0);
    CHECK(m.element(2) == 0);
    CHECK(m.element(3) == 2);
    CHECK(m.element(4) == 1);
    CHECK(m.element(5) == 0);
    CHECK(m.element(6) == 3);
    CHECK(m.element(7) == 2);
    CHECK(m.element(8) == 1);

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

    Mat3 m3 = Mat3::rotate_x(90);
    p2 = m3 * p1;
    CHECK(p2 == Vec3(2, -3, -2));

    m3 = Mat3::rotate_y(90);
    p2 = m3 * p1;
    CHECK(p2 == Vec3(3, -2, -2));

    m3 = Mat3::rotate_z(90);
    p2 = m3 * p1;
    CHECK(p2 == Vec3(2, 2, 3));

    p1 = Vec3(5.0f, -4.0f, 1.0f);
    m3 = Mat3::translate_2d(Vec2(10.0f, 20.0f));
    p2 = m3 * p1;
    CHECK(p2 == Vec3(15.0f, 16.0f, 1.0f));

    m3 = Mat3::scale_2d(Vec2(-1.0f, 0.5f));
    p2 = m3 * p2;
    CHECK(p2 == Vec3(-15.0f, 8.0f, 1.0f));

    p2 = Vec3(3.0f, 4.0f, 1.0f);
    m3 = Mat3::rotate_2d(LD_TO_RADIANS(90.0f));
    p2 = m3 * p2;
    CHECK(p2 == Vec3(-4.0f, 3.0f, 1.0f));
}

TEST_CASE("Mat3 decomposition")
{
    Vec3 euler;
    Mat3 rot = Mat3::rotate_x(90);
    bool ok = decompose_mat3_rot(rot, euler);
    CHECK(ok);
    CHECK(euler == Vec3(90, 0, 0));

    rot = Mat3::rotate_y(90);
    ok = decompose_mat3_rot(rot, euler);
    CHECK(ok);
    CHECK(euler == Vec3(0, 90, 0));

    rot = Mat3::rotate_z(90);
    ok = decompose_mat3_rot(rot, euler);
    CHECK(ok);
    CHECK(euler == Vec3(0, 0, 90));

    {
        rot = Mat4::rotate(LD_PI_2, Vec3(1.0f, 0.0f, 0.0f)).as_mat3();
        ok = decompose_mat3_rot(rot, euler);
        CHECK(ok);
        CHECK(euler == Vec3(90, 0, 0));

        rot = Mat4::rotate(LD_PI_2, Vec3(0.0f, 1.0f, 0.0f)).as_mat3();
        ok = decompose_mat3_rot(rot, euler);
        CHECK(ok);
        CHECK(euler == Vec3(0, 90, 0));

        rot = Mat4::rotate(LD_PI_2, Vec3(0.0f, 0.0f, 1.0f)).as_mat3();
        ok = decompose_mat3_rot(rot, euler);
        CHECK(ok);
        CHECK(euler == Vec3(0, 0, 90));
    }

    {
        rot = Mat4::rotate(LD_TO_RADIANS(30), Vec3(1.0f, 0.0f, 0.0f)).as_mat3();
        ok = decompose_mat3_rot(rot, euler);
        CHECK(ok);
        CHECK(euler == Vec3(30, 0, 0));

        rot = Mat4::rotate(LD_TO_RADIANS(30), Vec3(0.0f, 1.0f, 0.0f)).as_mat3();
        ok = decompose_mat3_rot(rot, euler);
        CHECK(ok);
        CHECK(euler == Vec3(0, 30, 0));

        rot = Mat4::rotate(LD_TO_RADIANS(30), Vec3(0.0f, 0.0f, 1.0f)).as_mat3();
        ok = decompose_mat3_rot(rot, euler);
        CHECK(ok);
        CHECK(euler == Vec3(0, 0, 30));
    }

    {
        rot = Mat4::rotate(LD_TO_RADIANS(270), Vec3(1.0f, 0.0f, 0.0f)).as_mat3();
        ok = decompose_mat3_rot(rot, euler);
        CHECK(ok);
        CHECK(euler == Vec3(270, 0, 0));

        rot = Mat4::rotate(LD_TO_RADIANS(270), Vec3(0.0f, 1.0f, 0.0f)).as_mat3();
        ok = decompose_mat3_rot(rot, euler);
        CHECK(ok);
        CHECK(euler == Vec3(0, 270, 0));

        rot = Mat4::rotate(LD_TO_RADIANS(270), Vec3(0.0f, 0.0f, 1.0f)).as_mat3();
        ok = decompose_mat3_rot(rot, euler);
        CHECK(ok);
        CHECK(euler == Vec3(0, 0, 270));
    }
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

    Vec4 p2 = Mat4::translate({2, -1, 3}) * p1;
    CHECK(p2 == Vec4(5, 1, 7, 1));

    p2 = Mat4::scale({3, -2, 4}) * p1;
    CHECK(p2 == Vec4(9, -4, 16, 1));

    Vec4 p3 = Mat4::rotate(LD_PI_2, Vec3(0, 0, 1)) * Vec4(2, 10, 7, 1);
    CHECK(p3 == Vec4(-10, 2, 7, 1));

    Mat4 mi = Mat4::inverse(Mat4(1.0f));
    CHECK(mi[0] == Vec4(1.0f, 0.0f, 0.0f, 0.0f));
    CHECK(mi[1] == Vec4(0.0f, 1.0f, 0.0f, 0.0f));
    CHECK(mi[2] == Vec4(0.0f, 0.0f, 1.0f, 0.0f));
    CHECK(mi[3] == Vec4(0.0f, 0.0f, 0.0f, 1.0f));

    m = Mat4({4, 7, 2, 0},
             {3, 6, 1, 0},
             {2, 5, 9, 0},
             {1, 0, 0, 1});

    CHECK(m.element(0) == 4);
    CHECK(m.element(1) == 7);
    CHECK(m.element(2) == 2);
    CHECK(m.element(3) == 0);
    CHECK(m.element(4) == 3);
    CHECK(m.element(5) == 6);
    CHECK(m.element(6) == 1);
    CHECK(m.element(7) == 0);
    CHECK(m.element(8) == 2);
    CHECK(m.element(9) == 5);
    CHECK(m.element(10) == 9);
    CHECK(m.element(11) == 0);
    CHECK(m.element(12) == 1);
    CHECK(m.element(13) == 0);
    CHECK(m.element(14) == 0);
    CHECK(m.element(15) == 1);

    Mat4 m3 = Mat4::inverse(m);
    Mat4 m4 = m3 * m;
    CHECK(m4[0] == Vec4(1.0f, 0.0f, 0.0f, 0.0f));
    CHECK(m4[1] == Vec4(0.0f, 1.0f, 0.0f, 0.0f));
    CHECK(m4[2] == Vec4(0.0f, 0.0f, 1.0f, 0.0f));
    CHECK(m4[3] == Vec4(0.0f, 0.0f, 0.0f, 1.0f));
}

TEST_CASE("Transform decomposition")
{
    Mat4 mat = Mat4::rotate(LD_TO_RADIANS(90), Vec3(0.0f, 1.0f, 0.0f));
    TransformEx t;
    bool ok = decompose_mat4_to_transform(mat, t);
    CHECK(ok);
    CHECK(t.position == Vec3(0.0f, 0.0f, 0.0f));
    CHECK(t.rotationEuler == Vec3(0.0f, 90.0f, 0.0f));
    CHECK(t.scale == Vec3(1.0f, 1.0f, 1.0f));
    CHECK(t.rotation.is_normalized());

    mat = mat * Mat4::scale(Vec3(1.0f, 2.0f, 3.0f));
    ok = decompose_mat4_to_transform(mat, t);
    CHECK(ok);
    CHECK(t.position == Vec3(0.0f, 0.0f, 0.0f));
    CHECK(t.rotationEuler == Vec3(0.0f, 90.0f, 0.0f));
    CHECK(t.scale == Vec3(1.0f, 2.0f, 3.0f));
    CHECK(t.rotation.is_normalized());

    mat = Mat4::translate(Vec3(-3.0f, 4.0f, 5.0f)) * mat;
    ok = decompose_mat4_to_transform(mat, t);
    CHECK(ok);
    CHECK(t.position == Vec3(-3.0f, 4.0f, 5.0f));
    CHECK(t.rotationEuler == Vec3(0.0f, 90.0f, 0.0f));
    CHECK(t.scale == Vec3(1.0f, 2.0f, 3.0f));
    CHECK(t.rotation.is_normalized());
}
#pragma once

#include <Extra/doctest/doctest.h>
#include <Ludens/Header/Math/Geometry.h>
using namespace LD;

TEST_CASE("Geometry Ray")
{
    Ray r0(Vec3(2.0f), Vec3(1.0f, -2.0f, 3.0f));
    r0.normalize();
    float len = r0.dir.length();
    CHECK(r0.origin == Vec3(2.0f));
    CHECK(is_zero_epsilon(len - 1.0f));
}

TEST_CASE("Geometry nearest")
{
    Ray r0(Vec3(0.0f), Vec3(0.0f, 1.0f, 0.0f));
    Ray r1(Vec3(3.0f), Vec3(0.0f, -1.0f, 0.0f));

    float t0, t1;
    bool ok = geometry_nearest(r0, r1, t0, t1);
    CHECK(!ok); // both parallel to Y axis

    ok = geometry_nearest(r1, r1, t0, t1);
    CHECK(!ok); // same ray, parallel

    r0 = Ray(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));
    r1 = Ray(Vec3(1.0f, 0.0f, 5.0f), Vec3(1.0f, 0.0f, 0.0f));
    ok = geometry_nearest(r0, r1, t0, t1);
    CHECK(ok); // intersecting at (0, 0, 5)
    CHECK(t0 == 5.0f);
    CHECK(t1 == -1.0f);
    Vec3 p0 = r0.parametric(t0);
    Vec3 p1 = r1.parametric(t1);
    CHECK(p0 == Vec3(0.0f, 0.0f, 5.0f));
    CHECK(p1 == Vec3(0.0f, 0.0f, 5.0f));

    r0 = Ray(Vec3(0.0f, 0.0f, -10.0f), Vec3(0.0f, 0.0f, 1.0f)); // Z axis
    r1 = Ray(Vec3(5.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f));
    ok = geometry_nearest(r0, r1, t0, t1);
    CHECK(ok);
    CHECK(t0 == 10.0f);
    CHECK(t1 == -5.0f);
    p0 = r0.parametric(t0);
    p1 = r1.parametric(t1);
    CHECK(p0 == Vec3(0.0f, 0.0f, 0.0f));
    CHECK(p1 == Vec3(0.0f, 1.0f, 0.0f));
}

TEST_CASE("Geometry ray-plane intersection")
{
    Ray r0;
    r0.origin = Vec3(1.0f, 2.0f, 0.0f);
    r0.dir = Vec3(0.0f, 0.0f, 1.0f);
    Plane plane; // plane at z = 5
    plane.point = Vec3(0.0f, 0.0f, 5.0f);
    plane.dir = Vec3(0.0f, 0.0f, -1.0f);

    // ray intersects with plane
    float t;
    bool ok = geometry_intersects(plane, r0, t);
    CHECK(ok);
    Vec3 p0 = r0.parametric(t);
    CHECK(p0 == Vec3(1.0f, 2.0f, 5.0f));

    // ray is parallel to plane
    r0.dir = Vec3(0.0f, 1.0f, 0.0f);
    ok = geometry_intersects(plane, r0, t);
    CHECK(!ok);

    // ray intersects behind the origin (negative parametric t)
    plane.point = Vec3(0.0f, 0.0f, -5.0f);
    plane.dir = Vec3(0.0f, 0.0f, 1.0f);
    r0.origin = Vec3(1.0f, 2.0f, 3.0f);
    r0.dir = Vec3(0.0f, 0.0f, 1.0f);
    ok = geometry_intersects(plane, r0, t);
    CHECK(ok);
    CHECK(t == -8.0f);
    p0 = r0.parametric(t);
    CHECK(p0 == Vec3(1.0f, 2.0f, -5.0f));

    // ray origin is on the plane (t = 0)
    plane.point = Vec3(0.0f);
    plane.dir = Vec3(0.0f, 1.0f, 0.0f);
    r0.origin = Vec3(0.0f);
    r0.dir = Vec3(0.0f, 1.0f, 0.0f);
    ok = geometry_intersects(plane, r0, t);
    CHECK(ok);
    CHECK(is_zero_epsilon(t));

    // ray intersects with tilted plane
    plane.point = Vec3(0.0f);
    plane.dir = Vec3::normalize(Vec3(0.0f, 1.0f, 1.0f));
    r0.origin = Vec3(0.0f, -2.0f, 0.0f);
    r0.dir = Vec3(0.0f, 1.0f, 0.0f);
    ok = geometry_intersects(plane, r0, t);
    CHECK(ok);
    CHECK(t == 2.0f);
    p0 = r0.parametric(t);
    CHECK(p0 == Vec3(0.0f));
}
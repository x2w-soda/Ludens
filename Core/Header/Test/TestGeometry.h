#pragma once

#include <Extra/doctest/doctest.h>
#include <Ludens/Header/Math/Geometry.h>
using namespace LD;

TEST_CASE("Geometry Ray")
{
    Ray r0(Vec3(2.0f), Vec3(1.0f, -2.0f, 3.0f));
    r0.normalize();
    float len = r0.dir.length();
    CHECK(r0.begin == Vec3(2.0f));
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
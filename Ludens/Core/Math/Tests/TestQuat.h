#pragma once

#include <doctest.h>
#include "Core/Math/Include/Quat.h"

using namespace LD;

static bool Equal(float lhs, float rhs)
{
    return LD_MATH_ABS(lhs - rhs) <= LD_MATH_TOLERANCE;
}

TEST_CASE("Identity")
{
    Quat q(Quat::Identity);

    CHECK(q.IsNormalized());
    CHECK(q.w == 1.0f);

    Vec3 axis;
    Radians rad;
    q.GetAxisRadians(axis, rad);

    CHECK(Equal(rad, 0.0f));
    CHECK(Equal(axis.x, 0.0f));
    CHECK(Equal(axis.y, 0.0f));
    CHECK(Equal(axis.z, 0.0f));
}

TEST_CASE("Quaternion Axis-Radian")
{
    {
        Vec3 axis(0.0f, 1.0f, 0.0f);
        Radians rad(LD_MATH_PI / 2.0f);
        Quat q = Quat::FromAxisRadians(axis, rad);

        q.GetAxisRadians(axis, rad);
        CHECK(Equal(rad, LD_MATH_PI / 2.0f));
        CHECK(Equal(axis.x, 0.0f));
        CHECK(Equal(axis.y, 1.0f));
        CHECK(Equal(axis.z, 0.0f));
    }

    {
        Vec3 axis_ = Vec3(3.14f, 2.71f, 6.67f).Normalized();
        Vec3 axis;
        Radians rad(LD_MATH_PI / 8.0f);
        Quat q = Quat::FromAxisRadians(axis_, rad);

        q.GetAxisRadians(axis, rad);
        CHECK(Equal(rad, LD_MATH_PI / 8.0f));
        CHECK(Equal(axis.x, axis_.x));
        CHECK(Equal(axis.y, axis_.y));
        CHECK(Equal(axis.z, axis_.z));
    }
}
#pragma once

#include <Ludens/Header/Math/Math.h>
#include <Ludens/Header/Math/Vec3.h>

namespace LD {

template <typename T>
struct TRay
{
    TVec3<T> origin; /// origin of ray
    TVec3<T> dir;    /// user keeps direction normalized

    inline void normalize()
    {
        dir = TVec3<T>::normalize(dir);
    }

    inline TVec3<T> parametric(T t) const
    {
        return origin + dir * t;
    }
};

using Ray = TRay<float>;

template <typename T>
struct TPlane
{
    TVec3<T> point; /// arbitrary point on the plane
    TVec3<T> dir;   /// user keeps plane-normal direction normalized
};

using Plane = TPlane<float>;

template <typename T>
struct TCapsule
{
    TVec3<T> begin;
    TVec3<T> end;
    T radius;
};

using Capsule = TCapsule<float>;

template <typename T>
bool geometry_nearest(const TRay<T>& r0, const TRay<T>& r1, float& t0, float& t1)
{
    // solve for t0, t1 such that length((r0.origin + r0.dir * t0) - (r1.origin + r1.dir * t1)) is minimal.

    TVec3<T> w = r0.origin - r1.origin;
    T q = TVec3<T>::dot(r0.dir, r1.dir);
    T s = TVec3<T>::dot(r1.dir, w);
    T r = TVec3<T>::dot(r0.dir, w);
    T denom = 1.0f - q * q;

    if (is_zero_epsilon<T>(denom))
    {
        // two rays are parallel
        return false;
    }

    t0 = static_cast<float>((q * s - r) / denom);
    t1 = static_cast<float>((s - q * r) / denom);

    return true;
}

template <typename T>
bool geometry_intersects(const TPlane<T>& plane, const TRay<T>& ray, float& t)
{
    T denom = TVec3<T>::dot(plane.dir, ray.dir);

    if (is_zero_epsilon<T>(denom))
    {
        // ray is parallel to plane
        t = 0.0f;
        return false;
    }

    // solve for t such that ray.parametric(t) is the intersection point
    t = TVec3<T>::dot(plane.point - ray.origin, plane.dir) / denom;

    // if t is negative, the intersection happens behind the ray origin
    return true;
}

} // namespace LD
#pragma once

#include <Ludens/Header/Math/Math.h>
#include <Ludens/Header/Math/Vec3.h>

namespace LD {

template <typename T>
struct TRay
{
    TVec3<T> begin; /// origin of ray
    TVec3<T> dir;   /// user keeps normalized

    inline void normalize()
    {
        dir = TVec3<T>::normalize(dir);
    }

    inline TVec3<T> parametric(T t) const
    {
        return begin + dir * t;
    }
};

using Ray = TRay<float>;

template <typename T>
struct TPlane
{
    TVec3<T> offset;
    TVec3<T> dir;
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

    TVec3<T> w = r0.begin - r1.begin;
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

} // namespace LD
#pragma once

#include "Core/Header/Include/Error.h"
#include "Core/Math/Include/Math.h"
#include "Core/Math/Include/Vec3.h"
#include "Core/Math/Include/Vec4.h"
#include "Core/Math/Include/Mat4.h"

namespace LD
{

/// Quaternion with imaginary part x,y,z and real part w
template <typename T>
struct TQuat : public TVec4<T>
{
    TQuat() = default;
    TQuat(T x, T y, T z, T w) : TVec4(x, y, z, w)
    {
    }

    TQuat(const TVec3<T>& v, T w) : TVec4(v.x, v.y, v.z, w)
    {
    }

    TQuat<T> Conjugated() const
    {
        return { -x, -y, -z, w };
    }

    TQuat<T> Normalized() const
    {
        TVec4<T> v = TVec4<T>::Normalized();
        return { v.x, v.y, v.z, v.w };
    }

    TQuat<T> operator*(const TQuat<T>& other) const
    {
        float lx = x;
        float ly = y;
        float lz = z;
        float lw = w;

        float rx = other.x;
        float ry = other.y;
        float rz = other.z;
        float rw = other.w;

        float x = lw * rx + lx * rw + ly * rz - lz * ry;
        float y = lw * ry - lx * rz + ly * rw + lz * rx;
        float z = lw * rz + lx * ry - ly * rx + lz * rw;
        float w = lw * rw - lx * rx - ly * ry - lz * rz;

        return { x, y, z, w };
    }

    /// @brief rotate a vector
    /// @warn quaternion must first be normalized
    TVec3<T> operator*(const TVec3<T>& vec) const
    {
        LD_DEBUG_ASSERT(IsNormalized());

        TQuat<T> p = *this * TQuat(vec, 0) * Conjugated();
        return { p.x, p.y, p.z };
    }

    void GetAxisRadians(TVec3<T>& axis, TRadians<T>& rad) const
    {
        LD_DEBUG_ASSERT(IsNormalized());

        // positive real part
        TQuat<T> q = (w > (T)0) ? TQuat<T>{ x, y, z, w } : TQuat<T>{ -x, -y, -z, -w };

        if (q.w >= (T)1)
        {
            axis = TVec3<T>::Zero;
            rad = (T)0;
        }
        else
        {
            rad = (T)2 * LD_MATH_ACOS(q.w);
            axis = TVec3{ q.x, q.y, q.z }.NormalizedOrZero();
        }
    }

    void GetMat4(TMat4<T>& mat) const
    {
        LD_DEBUG_ASSERT(IsNormalized());

        TVec3<T> axis;
        TRadians<T> rad;
        GetAxisRadians(axis, rad);

        mat = TMat4<T>::Rotate(axis, rad.ToDegrees());
    }

    static TQuat FromAxisRadians(const TVec3<T>& axis, const TRadians<T>& rads)
    {
        LD_DEBUG_ASSERT(axis.IsNormalized());

        T s = LD_MATH_SIN(rads / (T)2);
        return TQuat{ axis.x * s, axis.y * s, axis.z * s, LD_MATH_COS(rads / (T)2) };
    }

    static const TQuat<T> Identity;
};

template <typename T>
const TQuat<T> TQuat<T>::Identity{ T(0), T(0), T(0), T(1) };

using Quat = TQuat<float>;

} // namespace LD
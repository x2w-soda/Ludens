#pragma once

#include <Ludens/Header/Math/Mat3.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/Header/Math/Math.h>
#include <Ludens/Header/Math/Vec3.h>
#include <Ludens/Header/Types.h>
#include <algorithm>

namespace LD {

/// @brief Quaternion. A unit quaternion can be used to represent a rotation in 3D space.
template <typename T>
struct TQuat
{
    T x, y, z, w;

    TQuat() = default;
    TQuat(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    TQuat(const TVec3<T>& v, T w) : x(v.x), y(v.y), z(v.z), w(w) {}

    /// @brief Get length of quaternion.
    /// @return Scalar length.
    inline T length() const
    {
        return (T)LD_SQRT(w * w + x * x + y * y + z * z);
    }

    /// @brief Normalize a quaternion such that it has length 1.
    /// @warning Does not check for zero length division.
    static inline TQuat normalize(const TQuat<T>& q)
    {
        float l = q.length();

        return TQuat(q.x / l, q.y / l, q.z / l, q.w / l);
    }

    /// @brief Get a conjugated quaternion.
    static inline TQuat conjugate(const TQuat<T>& q)
    {
        return TQuat(-q.x, -q.y, -q.z, q.w);
    }

    inline bool is_normalized() const
    {
        return is_equal_epsilon<T>(length(), (T)1);
    }

    /// @brief Get a vec3 by dropping the real part.
    inline TVec3<T> as_vec3() const
    {
        return TVec3<T>(x, y, z);
    }

    /// @brief Get euler angle vector.
    /// @return Euler angle vector, rotation in degrees.
    /// @warning Must be a unit quaternion to represent a rotation in 3D.
    inline TVec3<T> as_euler() const
    {
        LD_ASSERT(is_normalized());

        T radX = LD_ATAN2(2 * (w * x + y * z), w * w - x * x - y * y + z * z /*1 - 2*(x*x + y*y)*/);
        T radY = LD_ASIN(std::clamp<T>((T)2 * (w * y - x * z), (T)-1, (T)1));
        T radZ = LD_ATAN2(2 * (w * z + x * y), w * w + x * x - y * y - z * z /*1 - 2*(y*y + z*z*/);

        return TVec3<T>(LD_TO_DEGREES(radX), LD_TO_DEGREES(radY), LD_TO_DEGREES(radZ));
    }

    /// @brief Get rotation matrix as Mat3.
    inline TMat3<T> as_mat3() const
    {
        T xx = x * x;
        T yy = y * y;
        T zz = z * z;
        T xy = x * y;
        T xz = x * z;
        T yz = y * z;
        T wx = w * x;
        T wy = w * y;
        T wz = w * z;

        return TMat3<T>(TVec3<T>((T)1 - (T)2 * (yy + zz), (T)2 * (xy + wz), (T)2 * (xz - wy)),
                        TVec3<T>((T)2 * (xy - wz), (T)1 - (T)2 * (xx + zz), (T)2 * (yz + wx)),
                        TVec3<T>((T)2 * (xz + wy), (T)2 * (yz - wx), (T)1 - (T)2 * (xx + yy)));
    }

    /// @brief Get rotation matrix as Mat4.
    inline TMat4<T> as_mat4() const
    {
        return TMat4<T>(as_mat3(), (T)1);
    }

    /// @brief create from array of 4 scalar elements, the last element being the real part
    template <typename TElement>
    static TQuat from_data(const TElement* a)
    {
        T x = static_cast<T>(a[0]);
        T y = static_cast<T>(a[1]);
        T z = static_cast<T>(a[2]);
        T w = static_cast<T>(a[3]);
        return {x, y, z, w};
    }

    static TQuat from_euler(const TVec3<T>& eulerDeg)
    {
        T roll = LD_TO_RADIANS(eulerDeg.x);
        T pitch = LD_TO_RADIANS(eulerDeg.y);
        T yaw = LD_TO_RADIANS(eulerDeg.z);

        T cr = (T)LD_COS(roll * (T)0.5);
        T sr = (T)LD_SIN(roll * (T)0.5);
        T cp = (T)LD_COS(pitch * (T)0.5);
        T sp = (T)LD_SIN(pitch * (T)0.5);
        T cy = (T)LD_COS(yaw * (T)0.5);
        T sy = (T)LD_SIN(yaw * (T)0.5);

        TQuat q;
        q.x = cy * cp * sr - sy * sp * cr;
        q.y = cy * sp * cr + sy * cp * sr;
        q.z = sy * cp * cr - cy * sp * sr;
        q.w = cy * cp * cr + sy * sp * sr;
        return q;
    }

    static TQuat from_axis_angle(const TVec3<T>& axis, T angleRad)
    {
        LD_ASSERT(is_equal_epsilon<T>(axis.length(), 1));

        const T angleRad2 = angleRad / (T)2;

        return TQuat(axis * LD_SIN(angleRad2), LD_COS(angleRad2));
    }

    static TQuat from_mat3(const TMat3<T>& m)
    {
        TVec3<T> euler;
        if (!decompose_mat3_rot(m, euler))
            return {};

        return TQuat::from_euler(euler);
    }
};

template <typename T>
inline TQuat<T> operator*(const TQuat<T>& lhs, const TQuat<T>& rhs)
{
    T lx = lhs.x;
    T ly = lhs.y;
    T lz = lhs.z;
    T lw = lhs.w;
    T rx = rhs.x;
    T ry = rhs.y;
    T rz = rhs.z;
    T rw = rhs.w;

    T x = lw * rx + lx * rw + ly * rz - lz * ry;
    T y = lw * ry - lx * rz + ly * rw + lz * rx;
    T z = lw * rz + lx * ry - ly * rx + lz * rw;
    T w = lw * rw - lx * rx - ly * ry - lz * rz;
    return TQuat<T>(x, y, z, w);
}

/// @brief Rotate a position using quaternion.
template <typename T>
inline TVec3<T> operator*(const TQuat<T>& lhs, const TVec3<T>& rhs)
{
    LD_ASSERT(lhs.is_normalized());

    TQuat<T> q = lhs * TQuat<T>(rhs, 0) * TQuat<T>::conjugate(lhs);
    return q.as_vec3();
}

using Quat = TQuat<float>;

static_assert(LD::IsTrivial<Quat>);

} // namespace LD
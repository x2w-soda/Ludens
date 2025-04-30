#pragma once

#include <Ludens/Header/Math/Math.h>
#include <Ludens/Header/Math/Vec4.h>
#include <Ludens/Header/Math/Vec3.h>

namespace LD {

/// @brief 4x4 matrix, containing 4 column vectors
template <typename T>
struct TMat4
{
    using TVec = TVec4<T>;

    TVec col[4];

    TMat4() : col{} {}
    TMat4(const TVec& c0, const TVec& c1, const TVec& c2, const TVec& c3) : col{c0, c1, c2, c3} {}
    TMat4(T x) : col{TVec(x, 0, 0, 0), TVec(0, x, 0, 0), TVec(0, 0, x, 0), TVec(0, 0, 0, x)} {}

    inline TVec& operator[](int i) { return col[i]; }
    inline const TVec& operator[](int i) const { return col[i]; }

    /// @brief create translation matrix
    static inline TMat4<T> translate(const TVec3<T>& offset)
    {
        TMat4<T> translation((T)1);

        translation[3] = TVec4<T>(offset.x, offset.y, offset.z, (T)1);

        return translation;
    }

    /// @brief create rotation matrix
    /// @param radians rotation angle in radians
    /// @param axis rotation axis, caller ensures this is a unit vector
    static inline TMat4<T> rotate(T radians, const TVec3<T>& axis)
    {
        const T c = LD_COS(radians);
        const T s = LD_SIN(radians);
        const TVec3<T> temp(axis * (static_cast<T>(1) - c));

        TMat4<T> rotation((T)1);

        rotation[0].x = c + temp.x * axis.x;
        rotation[0].y = temp.x * axis.y + s * axis.z;
        rotation[0].z = temp.x * axis.z - s * axis.y;
        rotation[1].x = temp.y * axis.x - s * axis.z;
        rotation[1].y = c + temp.y * axis.y;
        rotation[1].z = temp.y * axis.z + s * axis.x;
        rotation[2].x = temp.z * axis.x + s * axis.y;
        rotation[2].y = temp.z * axis.y - s * axis.x;
        rotation[2].z = c + temp.z * axis.z;

        return rotation;
    }

    /// @brief create scale matrix
    static inline TMat4<T> scale(const TVec3<T>& axis)
    {
        TMat4<T> scale((T)1);

        scale[0].x = axis.x;
        scale[1].y = axis.y;
        scale[2].z = axis.z;

        return scale;
    }
};

template <typename T>
inline TVec4<T> operator*(const TMat4<T>& m, const TVec4<T>& v)
{
    return m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3] * v.w;
}

/// @brief mat4 multiplication, rhs is applied before lhs
template <typename T>
inline TMat4<T> operator*(const TMat4<T>& lhs, const TMat4<T>& rhs)
{
    return TMat4<T>(lhs * rhs[0], lhs * rhs[1], lhs * rhs[2], lhs * rhs[3]);
}

using Mat4 = TMat4<float>;
using IMat4 = TMat4<int>;

} // namespace LD
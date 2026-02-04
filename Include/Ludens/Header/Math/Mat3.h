#pragma once

#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Math.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Header/Math/Vec3.h>

namespace LD {

/// @brief 3x3 matrix, containing 3 column vectors
template <typename T>
struct TMat3
{
    using TVec = TVec3<T>;

    TVec col[3];

    TMat3() : col{} {}
    TMat3(const TVec& c0, const TVec& c1, const TVec& c2) : col{c0, c1, c2} {}
    TMat3(T x) : col{TVec(x, 0, 0), TVec(0, x, 0), TVec(0, 0, x)} {}
    TMat3(T m00, T m11, T m22) : col{TVec(m00, 0, 0), TVec(0, m11, 0), TVec(0, 0, m22)} {}

    inline TVec& operator[](int i) { return col[i]; }
    inline const TVec& operator[](int i) const { return col[i]; }

    inline T& element(int i) { return col[i / 3][i % 3]; }
    inline T element(int i) const { return col[i / 3][i % 3]; }

    /// @brief evaluate the deterimant of the matrix
    inline T det() const
    {
        T a00 = col[0].x, a01 = col[0].y, a02 = col[0].z;
        T a10 = col[1].x, a11 = col[1].y, a12 = col[1].z;
        T a20 = col[2].x, a21 = col[2].y, a22 = col[2].z;
        T det = a00 * (a11 * a22 - a12 * a21) - a01 * (a10 * a22 - a12 * a20) + a02 * (a10 * a21 - a11 * a20);

        return det;
    }

    /// @brief create a transposed 3x3 matrix
    static inline TMat3 transpose(const TMat3& m)
    {
        TMat3 t;

        t[0].x = m[0].x;
        t[0].y = m[1].x;
        t[0].z = m[2].x;

        t[1].x = m[0].y;
        t[1].y = m[1].y;
        t[1].z = m[2].y;

        t[2].x = m[0].z;
        t[2].y = m[1].z;
        t[2].z = m[2].z;

        return t;
    }

    /// @brief create an inverse matrix
    /// @warning does not check if matrix is reversible, crashes upon zero determinant
    static inline TMat3 inverse(const TMat3& m)
    {
        T a00 = m[0].x, a01 = m[0].y, a02 = m[0].z;
        T a10 = m[1].x, a11 = m[1].y, a12 = m[1].z;
        T a20 = m[2].x, a21 = m[2].y, a22 = m[2].z;
        T det = m.det();

        if (is_zero_epsilon<T>(det))
        {
            LD_UNREACHABLE; // crash in debug
            return TMat3(1);
        }

        T invDet = (T)1 / det;

        TMat3 inv;

        inv[0].x = +(a11 * a22 - a12 * a21) * invDet;
        inv[0].y = -(a01 * a22 - a02 * a21) * invDet;
        inv[0].z = +(a01 * a12 - a02 * a11) * invDet;

        inv[1].x = -(a10 * a22 - a12 * a20) * invDet;
        inv[1].y = +(a00 * a22 - a02 * a20) * invDet;
        inv[1].z = -(a00 * a12 - a02 * a10) * invDet;

        inv[2].x = +(a10 * a21 - a11 * a20) * invDet;
        inv[2].y = -(a00 * a21 - a01 * a20) * invDet;
        inv[2].z = +(a00 * a11 - a01 * a10) * invDet;

        return inv;
    }

    static inline TMat3 rotate_x(T degreesX)
    {
        T radX = LD_TO_RADIANS(degreesX);
        T c = LD_COS(radX);
        T s = LD_SIN(radX);
        return TMat3<T>(TVec(1, 0, 0), TVec(0, c, s), TVec(0, -s, c));
    }

    static inline TMat3 rotate_y(T degreesX)
    {
        T radX = LD_TO_RADIANS(degreesX);
        T c = LD_COS(radX);
        T s = LD_SIN(radX);
        return TMat3<T>(TVec(c, 0, -s), TVec(0, 1, 0), TVec(s, 0, c));
    }

    static inline TMat3 rotate_z(T degreesX)
    {
        T radX = LD_TO_RADIANS(degreesX);
        T c = LD_COS(radX);
        T s = LD_SIN(radX);
        return TMat3<T>(TVec(c, s, 0), TVec(-s, c, 0), TVec(0, 0, 1));
    }

    /// @brief Create translation matrix for homogeneous 2D.
    static inline TMat3 translate_2d(const TVec2<T>& offset)
    {
        TMat3<T> m((T)1);
        m[2].x = offset.x;
        m[2].y = offset.y;

        return m;
    }

    /// @brief Create rotation matrix for homogeneous 2D.
    /// @note For screen space with top-left origin, the rotation will appear clockwise.
    static inline TMat3 rotate_2d(T radians)
    {
        T c = (T)LD_COS(radians);
        T s = (T)LD_SIN(radians);

        return TMat3<T>(TVec3<T>(c, s, 0), TVec3<T>(-s, c, 0), TVec3<T>(0, 0, 1));
    }

    /// @brief Create scale matrix for homogeneous 2D.
    static inline TMat3 scale_2d(const TVec2<T>& scale)
    {
        return TMat3<T>(scale.x, scale.y, (T)1);
    }
};

template <typename T>
inline TVec3<T> operator*(const TMat3<T>& m, const TVec3<T>& v)
{
    return m[0] * v.x + m[1] * v.y + m[2] * v.z;
}

/// @brief mat3 multiplication, rhs is applied before lhs
template <typename T>
inline TMat3<T> operator*(const TMat3<T>& lhs, const TMat3<T>& rhs)
{
    return TMat3<T>(lhs * rhs[0], lhs * rhs[1], lhs * rhs[2]);
}

using Mat3 = TMat3<float>;
using IMat3 = TMat3<int>;
using DMat3 = TMat3<double>;

template <typename T>
inline bool decompose_mat3_rot(const TMat3<T>& m, TVec3<T>& rotation)
{
    T sy = -m[2].x;

    if (sy * sy > (T)1)
        return false;

    T cy = (T)LD_SQRT((T)1 - sy * sy);
    T x, y, z;

    if (is_zero_epsilon<T>(cy)) // gimbal lock
    {
        x = (T)LD_ATAN2(-m[1].z, m[1].y);
        y = (T)LD_ASIN(sy);
        z = (T)0;
    }
    else
    {
        x = (T)LD_ATAN2(m[2].y, m[2].z);
        y = (T)LD_ASIN(sy);
        z = (T)LD_ATAN2(m[1].x, m[0].x);
    }

    x = -(T)LD_TO_DEGREES(x);
    y = -(T)LD_TO_DEGREES(y);
    z = -(T)LD_TO_DEGREES(z);
    rotation.x = x < 0 ? x + (T)360 : x;
    rotation.y = y < 0 ? y + (T)360 : y;
    rotation.z = z < 0 ? z + (T)360 : z;
    return true;
}

} // namespace LD
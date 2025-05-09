#pragma once

#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Quat.h>
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

    inline TVec& operator[](int i) { return col[i]; }
    inline const TVec& operator[](int i) const { return col[i]; }

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

        T det = a00 * (a11 * a22 - a12 * a21) - a01 * (a10 * a22 - a12 * a20) + a02 * (a10 * a21 - a11 * a20);

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

} // namespace LD
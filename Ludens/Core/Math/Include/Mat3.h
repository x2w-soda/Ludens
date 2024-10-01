#pragma once

#include "Core/Header/Include/Types.h"
#include "Core/Math/Include/Math.h"
#include "Core/Math/Include/Vec3.h"
#include "Core/Math/Include/Mat4.h"

namespace LD
{

template <typename T>
class TMat3;
using Mat3 = TMat3<float>;
using IMat3 = TMat3<i32>;

#define LD_MAT3_SCALAR(OP)                                                                                             \
    template <typename T>                                                                                              \
    TMat3<T> inline operator OP(const TMat3<T>& m, T s)                                                                \
    {                                                                                                                  \
        return T##Mat3<T>(m[0] OP s, m[1] OP s, m[2] OP s);                                                            \
    }

LD_MAT3_SCALAR(+)
LD_MAT3_SCALAR(-)
LD_MAT3_SCALAR(*)
LD_MAT3_SCALAR(/)

template <typename T>
struct TMat3
{
    TMat3() : Data{}
    {
    }
    TMat3(const TVec3<T>& v1, const TVec3<T>& v2, const TVec3<T>& v3) : Data{ v1, v2, v3 }
    {
    }

    TMat3(const TMat4<T>& mat4)
    {
        Data[0] = { mat4[0].x, mat4[0].y, mat4[0].z };
        Data[1] = { mat4[1].x, mat4[1].y, mat4[1].z };
        Data[2] = { mat4[2].x, mat4[2].y, mat4[2].z };
    }

    TVec3<T> Data[3];

    inline T* GetData()
    {
        return (T*)Data->Data;
    }

    inline const T* GetData() const
    {
        return (T*)Data->Data;
    }

    inline TVec3<T>& operator[](int i)
    {
        return Data[i];
    }

    inline const TVec3<T>& operator[](int i) const
    {
        return Data[i];
    }

    T Det() const
    {
        const T* data = GetData();
        return data[0] * (data[4] * data[8] - data[7] * data[5]) -
               data[3] * (data[1] * data[8] - data[7] * data[2]) +
               data[6] * (data[1] * data[5] - data[4] * data[2]);
    }

    bool IsInvertible(T tolerance = (T)LD_MATH_TOLERANCE) const
    {
        return !LD_MATH_EQUAL_ZERO(Det());
    }

    static const TMat3<T> Identity;
    static const TMat3<T> Zero;

    static bool Equal(const TMat3<T>& lhs, const TMat3<T>& rhs);
    static TMat3<T> Transpose(const TMat3<T>& mat);
    static TMat3<T> Inverse(const TMat3<T>& mat);
};

template <typename T>
const TMat3<T> TMat3<T>::Identity{ { (T)1.0f, (T)0.0f, (T)0.0f },
                                   { (T)0.0f, (T)1.0f, (T)0.0f },
                                   { (T)0.0f, (T)0.0f, (T)1.0f } };

template <typename T>
const TMat3<T> TMat3<T>::Zero{ { (T)0.0f, (T)0.0f, (T)0.0f },
                               { (T)0.0f, (T)0.0f, (T)0.0f },
                               { (T)0.0f, (T)0.0f, (T)0.0f } };

template <typename T>
inline TVec3<T> operator*(const TMat3<T>& m, const TVec3<T>& v)
{
    return m[0] * v.x + m[1] * v.y + m[2] * v.z;
}

template <typename T>
inline TMat3<T> operator*(const TMat3<T>& lhs, const TMat3<T>& rhs)
{
    return TMat3<T>(lhs * rhs[0], lhs * rhs[1], lhs * rhs[2]);
}

template <typename T>
bool TMat3<T>::Equal(const TMat3<T>& lhs, const TMat3<T>& rhs)
{
    const T* lhsData = lhs.GetData();
    const T* rhsData = rhs.GetData();

    for (int i = 0; i < 9; i++)
    {
        if (!LD_MATH_EQUAL(lhsData[i], rhsData[i]))
            return false;
    }

    return true;
}

template <typename T>
TMat3<T> TMat3<T>::Transpose(const TMat3<T>& mat)
{
    TMat3<T> transposed;
    transposed[0][0] = mat[0][0];
    transposed[0][1] = mat[1][0];
    transposed[0][2] = mat[2][0];
    transposed[1][0] = mat[0][1];
    transposed[1][1] = mat[1][1];
    transposed[1][2] = mat[2][1];
    transposed[2][0] = mat[0][2];
    transposed[2][1] = mat[1][2];
    transposed[2][2] = mat[2][2];

    return transposed;
}

template <typename T>
TMat3<T> TMat3<T>::Inverse(const TMat3<T>& mat)
{
    TMat3<T> inverse;
    T det = mat.Det();
    T* invData = inverse.GetData();
    const T* matData = mat.GetData();

    // any determinant that isn't exactly zero will pass
    LD_DEBUG_ASSERT(det != (T)0);
    T denominator = (T)1 / det;

    // adjugate matrix
    invData[0] = matData[4] * matData[8] - matData[7] * matData[5];
    invData[1] = matData[7] * matData[2] - matData[1] * matData[8];
    invData[2] = matData[1] * matData[5] - matData[4] * matData[2];

    invData[3] = matData[6] * matData[5] - matData[3] * matData[8];
    invData[4] = matData[0] * matData[8] - matData[6] * matData[2];
    invData[5] = matData[3] * matData[2] - matData[0] * matData[5];

    invData[6] = matData[3] * matData[7] - matData[6] * matData[4];
    invData[7] = matData[6] * matData[1] - matData[0] * matData[7];
    invData[8] = matData[0] * matData[4] - matData[3] * matData[1];

    // divide adjugate by determinant
    for (int i = 0; i < 9; ++i)
    {
        invData[i] *= denominator;
    }

    return inverse;
}

} // namespace LD
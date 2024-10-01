#pragma once

#include "Core/Math/Include/Vec4.h"

namespace LD
{

template <typename T>
struct TMat4;
using Mat4 = TMat4<float>;
using IMat4 = TMat4<i32>;

#define LD_MAT4_SCALAR(OP)                                                                                             \
    template <typename T>                                                                                              \
    TMat4<T> inline operator OP(const TMat4<T>& m, T s)                                                                \
    {                                                                                                                  \
        return T##Mat4<T>(m[0] OP s, m[1] OP s, m[2] OP s, m[3] OP s);                                                 \
    }

LD_MAT4_SCALAR(+)
LD_MAT4_SCALAR(-)
LD_MAT4_SCALAR(*)
LD_MAT4_SCALAR(/)

template <typename T>
struct TMat4
{
    TMat4() : Data{}
    {
    }
    TMat4(const TVec4<T>& v1, const TVec4<T>& v2, const TVec4<T>& v3, const TVec4<T>& v4) : Data{ v1, v2, v3, v4 }
    {
    }

    TVec4<T> Data[4];
    inline T* GetData()
    {
        return (T*)Data->Data;
    }
    inline const T* GetData() const
    {
        return (T*)Data->Data;
    }

    inline TVec4<T>& operator[](int i)
    {
        return Data[i];
    }
    inline const TVec4<T>& operator[](int i) const
    {
        return Data[i];
    }

    static const TMat4<T> Identity;
    static const TMat4<T> Zero;

    static TMat4<T> Translate(const TVec3<T>& offset);
    static TMat4<T> Rotate(const TVec3<T>& axis, const TDegrees<T>& degrees);
    static TMat4<T> Scale(const TVec3<T>& axis);
    static TMat4<T> LookAt(const TVec3<T>& position, const TVec3<T>& direction, const TVec3<T>& worldUp);
    static TMat4<T> Perspective(T fovRadians, T aspect, T zNear, T zFar);
    static TMat4<T> Orthographic(T left, T right, T bottom, T top, T near, T far);
};

template <typename T>
TMat4<T> TMat4<T>::Translate(const TVec3<T>& offset)
{
    TMat4<T> translation = TMat4<T>::Identity;

    translation[3][0] = offset.x;
    translation[3][1] = offset.y;
    translation[3][2] = offset.z;

    return translation;
}

template <typename T>
TMat4<T> TMat4<T>::Rotate(const TVec3<T>& axis, const TDegrees<T>& degrees)
{
    const T rad(degrees.ToRadians());
    const T c(LD_MATH_COS(rad));
    const T s(LD_MATH_SIN(rad));
    const TVec3<T> a(axis.NormalizedOrZero());
    const TVec3<T> temp(a * (static_cast<T>(1.0f) - c));

    TMat4<T> rotation = TMat4<T>::Identity;

    rotation[0][0] = c + temp.x * a.x;
    rotation[0][1] = temp.x * a.y + s * a.z;
    rotation[0][2] = temp.x * a.z - s * a.y;
    rotation[1][0] = temp.y * a.x - s * a.z;
    rotation[1][1] = c + temp.y * a.y;
    rotation[1][2] = temp.y * a.z + s * a.x;
    rotation[2][0] = temp.z * a.x + s * a.y;
    rotation[2][1] = temp.z * a.y - s * a.x;
    rotation[2][2] = c + temp.z * a.z;

    return rotation;
}

template <typename T>
TMat4<T> TMat4<T>::Scale(const TVec3<T>& axis)
{
    TMat4<T> scale = TMat4<T>::Identity;

    scale[0][0] = axis.x;
    scale[1][1] = axis.y;
    scale[2][2] = axis.z;

    return scale;
}

template <typename T>
TMat4<T> TMat4<T>::LookAt(const TVec3<T>& position, const TVec3<T>& direction, const TVec3<T>& worldUp)
{
    TVec3<T> d = direction.Normalized();
    TVec3<T> r = TVec3<T>::Cross(d, worldUp).Normalized();
    TVec3<T> u = TVec3<T>::Cross(r, d);

    TMat4<T> view = TMat4<T>::Identity;
    view[0][0] = r.x;
    view[1][0] = r.y;
    view[2][0] = r.z;
    view[0][1] = u.x;
    view[1][1] = u.y;
    view[2][1] = u.z;
    view[0][2] = -d.x;
    view[1][2] = -d.y;
    view[2][2] = -d.z;
    view[3][0] = -TVec3<T>::Dot(r, position);
    view[3][1] = -TVec3<T>::Dot(u, position);
    view[3][2] = TVec3<T>::Dot(d, position);
    return view;
}

template <typename T>
TMat4<T> TMat4<T>::Perspective(T fovRadians, T aspect, T zNear, T zFar)
{
    const T tanHalfFov = LD_MATH_TAN(fovRadians / static_cast<T>(2.0f));

    TMat4<T> projection = TMat4<T>::Zero;
    projection[0][0] = static_cast<T>(1.0f) / (aspect * tanHalfFov);
    projection[1][1] = static_cast<T>(1.0f) / tanHalfFov;
    projection[2][2] = (zNear + zFar) / (zNear - zFar);
    projection[2][3] = -static_cast<T>(1.0f);
    projection[3][2] = (static_cast<T>(2.0f) * zNear * zFar) / (zNear - zFar);

    return projection;
}

template <typename T>
TMat4<T> TMat4<T>::Orthographic(T left, T right, T bottom, T top, T near, T far)
{
    TMat4<T> ortho = TMat4<T>::Identity;
    ortho[0][0] = static_cast<T>(2.0f) / (right - left);
    ortho[1][1] = static_cast<T>(2.0f) / (top - bottom);
    ortho[2][2] = static_cast<T>(2.0f) / (near - far);
    ortho[3][0] = (right + left) / (left - right);
    ortho[3][1] = (top + bottom) / (bottom - top);
    ortho[3][2] = (near + far) / (near - far);
    return ortho;
}

template <typename T>
const TMat4<T> TMat4<T>::Identity{ { (T)1.0f, (T)0.0f, (T)0.0f, (T)0.0f },
                                   { (T)0.0f, (T)1.0f, (T)0.0f, (T)0.0f },
                                   { (T)0.0f, (T)0.0f, (T)1.0f, (T)0.0f },
                                   { (T)0.0f, (T)0.0f, (T)0.0f, (T)1.0f } };

template <typename T>
const TMat4<T> TMat4<T>::Zero{ { (T)0.0f, (T)0.0f, (T)0.0f, (T)0.0f },
                               { (T)0.0f, (T)0.0f, (T)0.0f, (T)0.0f },
                               { (T)0.0f, (T)0.0f, (T)0.0f, (T)0.0f },
                               { (T)0.0f, (T)0.0f, (T)0.0f, (T)0.0f } };

template <typename T>
inline TVec4<T> operator*(const TMat4<T>& m, const TVec4<T>& v)
{
    return m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3] * v.w;
}

template <typename T>
inline TMat4<T> operator*(const TMat4<T>& lhs, const TMat4<T>& rhs)
{
    return TMat4<T>(lhs * rhs[0], lhs * rhs[1], lhs * rhs[2], lhs * rhs[3]);
}

} // namespace LD
#pragma once

#include "Core/Header/Include/Types.h"
#include "Core/Math/Include/Math.h"
#include "Core/Math/Include/Vec3.h"
#include "Core/Math/Include/Vec2.h"

namespace LD
{

template <typename T>
struct TVec4;
using Vec4 = TVec4<float>;
using DVec4 = TVec4<double>;
using IVec4 = TVec4<i32>;

#define LD_VEC4_SCALAR(OP)                                                                                             \
    template <typename T>                                                                                              \
    inline TVec4<T> operator OP(const TVec4<T>& v, T s)                                                                \
    {                                                                                                                  \
        return TVec4<T>(v[0] OP s, v[1] OP s, v[2] OP s, v[3] OP s);                                                   \
    }

LD_VEC4_SCALAR(+)
LD_VEC4_SCALAR(-)
LD_VEC4_SCALAR(*)
LD_VEC4_SCALAR(/)

#define LD_VEC4_ARITH(OP)                                                                                              \
    template <typename T>                                                                                              \
    inline TVec4<T> operator OP(const TVec4<T>& lhs, const TVec4<T>& rhs)                                              \
    {                                                                                                                  \
        return TVec4<T>(lhs.x OP rhs.x, lhs.y OP rhs.y, lhs.z OP rhs.z, lhs.w OP rhs.w);                               \
    }

LD_VEC4_ARITH(+);
LD_VEC4_ARITH(-);
LD_VEC4_ARITH(*);
LD_VEC4_ARITH(/);

#define LD_VEC4_UNARY(OP)                                                                                              \
    template <typename T>                                                                                              \
    inline TVec4<T> operator OP(const TVec4<T>& self)                                                                  \
    {                                                                                                                  \
        return TVec4<T>(OP self.x, OP self.y, OP self.z, OP self.w);                                                   \
    }

LD_VEC4_UNARY(+);
LD_VEC4_UNARY(-);

template <typename T>
struct TVec4
{
    TVec4() : x(T(0)), y(T(0)), z(T(0)), w(T(0))
    {
    }
    TVec4(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_)
    {
    }
    TVec4(T x_, T y_, const TVec2<T>& v) : x(x_), y(y_), z(v.x), w(v.y)
    {
    }
    TVec4(T x_, const TVec2<T>& v, T w_) : x(x_), y(v.x), z(v.y), w(w_)
    {
    }
    TVec4(const TVec2<T>& v, T z_, T w_) : x(v.x), y(v.y), z(z_), w(w_)
    {
    }
    TVec4(const TVec2<T>& v1, const TVec2<T>& v2) : x(v1.x), y(v1.y), z(v2.x), w(v2.y)
    {
    }
    TVec4(const TVec3<T>& v, T w_) : x(v.x), y(v.y), z(v.z), w(w_)
    {
    }
    TVec4(T x_, const TVec3<T>& v) : x(x_), y(v.x), z(v.y), w(v.z)
    {
    }

    union
    {
        T Data[4];
        struct
        {
            union
            {
                T x;
                T r;
            };
            union
            {
                T y;
                T g;
            };
            union
            {
                T z;
                T b;
            };
            union
            {
                T w;
                T a;
            };
        };
    };

    inline T& operator[](int i)
    {
        return Data[i];
    }

    inline const T& operator[](int i) const
    {
        return Data[i];
    }

    /// exact float comparison
    inline bool operator==(const TVec4& other) const
    {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    static const TVec4<T> Zero;

    static inline T Dot(const TVec4<T>& v1, const TVec4<T>& v2)
    {
        return static_cast<T>(v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w);
    }

    inline T LengthSquared() const
    {
        return x * x + y * y + z * z + w * w;
    }

    inline T Length() const
    {
        return static_cast<T>(LD_MATH_SQRT(LengthSquared()));
    }

    inline TVec4<T> Normalized() const
    {
        T length = Length();
        LD_DEBUG_ASSERT(length != (T)0);

        return *this / length;
    }

    inline bool IsNormalized(float tolerance = LD_MATH_TOLERANCE) const
    {
        return LD_MATH_ABS(LengthSquared() - static_cast<T>(1)) <= tolerance;
    }

    /// @brief linear interpolate two vec4
    /// @note ratio is not clampled to [0, 1]
    static TVec4<T> Lerp(const TVec4<T>& from, const TVec4<T>& to, T ratio)
    {
        return from * (1 - ratio) + to * ratio;
    }

    // TODO: swizzle
};

template <typename T>
const TVec4<T> TVec4<T>::Zero{ T(0), T(0), T(0), T(0) };

} // namespace LD
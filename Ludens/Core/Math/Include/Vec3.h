#pragma once

#include "Core/Header/Include/Error.h"
#include "Core/Header/Include/Types.h"
#include "Core/Math/Include/Math.h"
#include "Core/Math/Include/Vec2.h"

namespace LD
{

template <typename T>
struct TVec3;
using Vec3 = TVec3<float>;
using DVec3 = TVec3<double>;
using IVec3 = TVec3<i32>;

#define LD_VEC3_SCALAR(OP)                                                                                             \
    template <typename T>                                                                                              \
    inline TVec3<T> operator OP(const TVec3<T>& v, T s)                                                                \
    {                                                                                                                  \
        return TVec3<T>(v[0] OP s, v[1] OP s, v[2] OP s);                                                              \
    }

LD_VEC3_SCALAR(+)
LD_VEC3_SCALAR(-)
LD_VEC3_SCALAR(*)
LD_VEC3_SCALAR(/)

#define LD_VEC3_ARITH(OP)                                                                                              \
    template <typename T>                                                                                              \
    inline TVec3<T> operator OP(const TVec3<T>& lhs, const TVec3<T>& rhs)                                              \
    {                                                                                                                  \
        return TVec3<T>(lhs.x OP rhs.x, lhs.y OP rhs.y, lhs.z OP rhs.z);                                               \
    }

LD_VEC3_ARITH(+);
LD_VEC3_ARITH(-);
LD_VEC3_ARITH(*);
LD_VEC3_ARITH(/);

#define LD_VEC3_UNARY(OP)                                                                                              \
    template <typename T>                                                                                              \
    inline TVec3<T> operator OP(const TVec3<T>& self)                                                                  \
    {                                                                                                                  \
        return TVec3<T>(OP self.x, OP self.y, OP self.z);                                                              \
    }

LD_VEC3_UNARY(+);
LD_VEC3_UNARY(-);

template <typename T>
struct TVec3
{
    TVec3() : x(T(0)), y(T(0)), z(T(0))
    {
    }
    TVec3(T x_, T y_, T z_) : x(x_), y(y_), z(z_)
    {
    }
    TVec3(const TVec2<T>& v, T z_) : x(v.x), y(v.y), z(z_)
    {
    }
    TVec3(T x_, const TVec2<T>& v) : x(x_), y(v.x), z(v.y)
    {
    }

    union
    {
        T Data[3];
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
    inline bool operator==(const TVec3& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }

    inline T LengthSquared() const
    {
        return x * x + y * y + z * z;
    }

    inline T Length() const
    {
        return static_cast<T>(LD_MATH_SQRT(LengthSquared()));
    }

    inline bool IsNormalized(float tolerance = LD_MATH_TOLERANCE) const
    {
        return LD_MATH_ABS(LengthSquared() - (T)1) <= tolerance;
    }

    inline TVec3<T> Normalized() const
    {
        T length = Length();
        LD_DEBUG_ASSERT(length != (T)0);

        return *this / length;
    }

    inline TVec3<T> NormalizedOrZero() const
    {
        T length = Length();

        return length <= LD_MATH_TOLERANCE ? TVec3<T>::Zero : (*this / length);
    }

    static const TVec3<T> Zero;

    static inline T Dot(const TVec3<T>& v1, const TVec3<T>& v2)
    {
        return static_cast<T>(v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
    }

    static inline TVec3<T> Cross(const TVec3<T>& v1, const TVec3<T>& v2)
    {
        return TVec3<T>(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
    }

#define LD_VEC3_SWIZZLE(RET_VEC, SWIZZLE, ...)                                                                         \
    inline RET_VEC<T> SWIZZLE() const                                                                                  \
    {                                                                                                                  \
        return RET_VEC<T>(__VA_ARGS__);                                                                                \
    }

    LD_VEC3_SWIZZLE(TVec3, xxx, x, x, x)
    LD_VEC3_SWIZZLE(TVec3, xxy, x, x, y)
    LD_VEC3_SWIZZLE(TVec3, xxz, x, x, z)
    LD_VEC3_SWIZZLE(TVec3, xyx, x, y, x)
    LD_VEC3_SWIZZLE(TVec3, xyy, x, y, y)
    LD_VEC3_SWIZZLE(TVec3, xyz, x, y, z)
    LD_VEC3_SWIZZLE(TVec3, xzx, x, z, x)
    LD_VEC3_SWIZZLE(TVec3, xzy, x, z, y)
    LD_VEC3_SWIZZLE(TVec3, xzz, x, z, z)
    LD_VEC3_SWIZZLE(TVec3, yxx, y, x, x)
    LD_VEC3_SWIZZLE(TVec3, yxy, y, x, y)
    LD_VEC3_SWIZZLE(TVec3, yxz, y, x, z)
    LD_VEC3_SWIZZLE(TVec3, yyx, y, y, x)
    LD_VEC3_SWIZZLE(TVec3, yyy, y, y, y)
    LD_VEC3_SWIZZLE(TVec3, yyz, y, y, z)
    LD_VEC3_SWIZZLE(TVec3, yzx, y, z, x)
    LD_VEC3_SWIZZLE(TVec3, yzy, y, z, y)
    LD_VEC3_SWIZZLE(TVec3, yzz, y, z, z)
    LD_VEC3_SWIZZLE(TVec3, zxx, z, x, x)
    LD_VEC3_SWIZZLE(TVec3, zxy, z, x, y)
    LD_VEC3_SWIZZLE(TVec3, zxz, z, x, z)
    LD_VEC3_SWIZZLE(TVec3, zyx, z, y, x)
    LD_VEC3_SWIZZLE(TVec3, zyy, z, y, y)
    LD_VEC3_SWIZZLE(TVec3, zyz, z, y, z)
    LD_VEC3_SWIZZLE(TVec3, zzx, z, z, x)
    LD_VEC3_SWIZZLE(TVec3, zzy, z, z, y)
    LD_VEC3_SWIZZLE(TVec3, zzz, z, z, z)

    LD_VEC3_SWIZZLE(TVec3, rrr, r, r, r)
    LD_VEC3_SWIZZLE(TVec3, rrg, r, r, g)
    LD_VEC3_SWIZZLE(TVec3, rrb, r, r, b)
    LD_VEC3_SWIZZLE(TVec3, rgr, r, g, r)
    LD_VEC3_SWIZZLE(TVec3, rgg, r, g, g)
    LD_VEC3_SWIZZLE(TVec3, rgb, r, g, b)
    LD_VEC3_SWIZZLE(TVec3, rbr, r, b, r)
    LD_VEC3_SWIZZLE(TVec3, rbg, r, b, g)
    LD_VEC3_SWIZZLE(TVec3, rbb, r, b, b)
    LD_VEC3_SWIZZLE(TVec3, grr, g, r, r)
    LD_VEC3_SWIZZLE(TVec3, grg, g, r, g)
    LD_VEC3_SWIZZLE(TVec3, grb, g, r, b)
    LD_VEC3_SWIZZLE(TVec3, ggr, g, g, r)
    LD_VEC3_SWIZZLE(TVec3, ggg, g, g, g)
    LD_VEC3_SWIZZLE(TVec3, ggb, g, g, b)
    LD_VEC3_SWIZZLE(TVec3, gbr, g, b, r)
    LD_VEC3_SWIZZLE(TVec3, gbg, g, b, g)
    LD_VEC3_SWIZZLE(TVec3, gbb, g, b, b)
    LD_VEC3_SWIZZLE(TVec3, brr, b, r, r)
    LD_VEC3_SWIZZLE(TVec3, brg, b, r, g)
    LD_VEC3_SWIZZLE(TVec3, brb, b, r, b)
    LD_VEC3_SWIZZLE(TVec3, bgr, b, g, r)
    LD_VEC3_SWIZZLE(TVec3, bgg, b, g, g)
    LD_VEC3_SWIZZLE(TVec3, bgb, b, g, b)
    LD_VEC3_SWIZZLE(TVec3, bbr, b, b, r)
    LD_VEC3_SWIZZLE(TVec3, bbg, b, b, g)
    LD_VEC3_SWIZZLE(TVec3, bbb, b, b, b)
};

template <typename T>
const TVec3<T> TVec3<T>::Zero{ T(0), T(0), T(0) };

} // namespace LD

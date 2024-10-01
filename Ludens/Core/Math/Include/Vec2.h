#pragma once

#include "Core/Header/Include/Types.h"
#include "Core/Math/Include/Math.h"

namespace LD
{

template <typename T>
struct TVec2;
using Vec2 = TVec2<float>;
using DVec2 = TVec2<double>;
using IVec2 = TVec2<i32>;

#define LD_VEC2_SCALAR(OP)                                                                                             \
    template <typename T>                                                                                              \
    inline TVec2<T> operator OP(const TVec2<T>& v, T s)                                                                \
    {                                                                                                                  \
        return TVec2<T>(v[0] OP s, v[1] OP s);                                                                         \
    }

LD_VEC2_SCALAR(+)
LD_VEC2_SCALAR(-)
LD_VEC2_SCALAR(*)
LD_VEC2_SCALAR(/)

#define LD_VEC2_ARITH(OP)                                                                                              \
    template <typename T>                                                                                              \
    inline TVec2<T> operator OP(const TVec2<T>& lhs, const TVec2<T>& rhs)                                              \
    {                                                                                                                  \
        return TVec2<T>(lhs.x OP rhs.x, lhs.y OP rhs.y);                                                               \
    }

LD_VEC2_ARITH(+);
LD_VEC2_ARITH(-);
LD_VEC2_ARITH(*);
LD_VEC2_ARITH(/);

#define LD_VEC2_UNARY(OP)                                                                                              \
    template <typename T>                                                                                              \
    inline TVec2<T> operator OP(const TVec2<T>& self)                                                                  \
    {                                                                                                                  \
        return TVec2<T>(OP self.x, OP self.y);                                                                         \
    }

LD_VEC2_UNARY(+);
LD_VEC2_UNARY(-);

template <typename T>
struct TVec2
{
    TVec2() : x(T(0)), y(T(0))
    {
    }
    TVec2(T x_, T y_) : x(x_), y(y_)
    {
    }

    union
    {
        T Data[2];
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
    inline bool operator==(const TVec2& other) const
    {
        return x == other.x && y == other.y;
    }

    inline T LengthSquared() const
    {
        return x * x + y * y;
    }
    
    inline T Length() const
    {
        return static_cast<T>(LD_MATH_SQRT(LengthSquared()));
    }

    inline TVec2<T> Normalized() const
    {
        T length = Length();
        LD_DEBUG_ASSERT(length != (T)0);

        return *this / length;
    }

    static const TVec2<T> Zero;

    static inline T Dot(const TVec2<T>& v1, const TVec2<T>& v2)
    {
        return static_cast<T>(v1.x * v2.x + v1.y * v2.y);
    }

#define LD_VEC2_SWIZZLE(RET_VEC, SWIZZLE, ...)                                                                         \
    inline RET_VEC<T> SWIZZLE() const                                                                                  \
    {                                                                                                                  \
        return RET_VEC<T>(__VA_ARGS__);                                                                                \
    }

    LD_VEC2_SWIZZLE(TVec2, xx, x, x)
    LD_VEC2_SWIZZLE(TVec2, xy, x, y)
    LD_VEC2_SWIZZLE(TVec2, yx, y, x)
    LD_VEC2_SWIZZLE(TVec2, yy, y, y)

    LD_VEC2_SWIZZLE(TVec2, rr, r, r)
    LD_VEC2_SWIZZLE(TVec2, rg, r, g)
    LD_VEC2_SWIZZLE(TVec2, gr, g, r)
    LD_VEC2_SWIZZLE(TVec2, gg, g, g)
};

template <typename T>
const TVec2<T> TVec2<T>::Zero{ T(0), T(0) };

} // namespace LD

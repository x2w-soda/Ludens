#pragma once

#include <Ludens/Header/Math/Math.h>
#include <Ludens/Header/Math/Vec3.h>
#include <Ludens/Header/SIMD.h>

#ifdef LD_SSE2
#define TVEC4_ALIGNMENT std::conditional_t<std::is_same_v<T, double>, std::integral_constant<size_t, 32>, std::conditional_t<std::is_same_v<T, float>, std::integral_constant<size_t, 16>, std::integral_constant<size_t, alignof(T)>>>::value
#else
#define TVEC4_ALIGNMENT std::integral_constant<size_t, alignof(T)>::value
#endif

namespace LD {

template <typename T>
struct alignas(TVEC4_ALIGNMENT) TVec4
{
    // clang-format off
    union
    {
        struct
        {
	        union { T x; T r; };
	        union { T y; T g; };
	        union { T z; T b; };
	        union { T w; T a; };
        };
#ifdef LD_SSE2
        std::conditional_t<std::is_same_v<T, double>, __m128d, __m128> data;
#endif
    };

    TVec4() = default;
    TVec4(T v) : x((T)v), y((T)v), z((T)v), w((T)v) {}
    TVec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    TVec4(const TVec2<T>& v1, const TVec2<T>& v2) : x(v1.x), y(v1.y), z(v2.x), w(v2.y) {}
	TVec4(const TVec3<T>& v, T w) : x(v.x), y(v.y), z(v.z), w(w) {}
	TVec4(T x, const TVec3<T>& v) : x(x), y(v.x), z(v.y), w(v.z) {}
#ifdef LD_SSE2
    TVec4(__m128 data) : data(data) {}
    TVec4(__m128d data) : data(data) {}
#endif
    // clang-format on

    /// @brief get a vec3 rvalue by dropping the w component
    inline TVec3<T> as_vec3() const { return {x, y, z}; }

    inline T length_squared() const { return x * x + y * y + z * z + w * w; }

    /// @brief get the length of the vector
    inline T length() const { return (T)LD_SQRT(x * x + y * y + z * z + w * w); }

    /// @brief create from array of 4 scalar elements
    template <typename TElement>
    static TVec4 from_data(const TElement* a)
    {
        T x = static_cast<T>(a[0]);
        T y = static_cast<T>(a[1]);
        T z = static_cast<T>(a[2]);
        T w = static_cast<T>(a[3]);
        return {x, y, z, w};
    }

    /// @brief dot product between two vectors
    static T dot(const TVec4& lhs, const TVec4& rhs)
    {
        return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
    }

    /// @brief normalize a vector
    /// @warning does not check for zero length division
    static TVec4 normalize(const TVec4& v)
    {
        return v / (T)LD_SQRT(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    }
};

/// @brief compare each component in vector, using epsilon tolerance for floating point precision
template <typename T>
inline bool operator==(const TVec4<T>& lhs, const TVec4<T>& rhs)
{
    if constexpr (std::is_same_v<T, float>)
    {
        return std::abs(lhs.x - rhs.x) < LD_EPSILON_F32 &&
               std::abs(lhs.y - rhs.y) < LD_EPSILON_F32 &&
               std::abs(lhs.z - rhs.z) < LD_EPSILON_F32 &&
               std::abs(lhs.w - rhs.w) < LD_EPSILON_F32;
    }
    else if constexpr (std::is_same_v<T, double>)
    {
        return std::abs(lhs.x - rhs.x) < LD_EPSILON_F64 &&
               std::abs(lhs.y - rhs.y) < LD_EPSILON_F64 &&
               std::abs(lhs.z - rhs.z) < LD_EPSILON_F64 &&
               std::abs(lhs.w - rhs.w) < LD_EPSILON_F64;
    }
    else
        return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

#define LD_VEC4_SCALAR(OP, SSE2, DSSE2)                              \
    template <typename T>                                            \
    inline TVec4<T> operator OP(const TVec4<T>& v, T s)              \
    {                                                                \
        if constexpr (std::is_same_v<T, float> && LD_SSE2)           \
            return SSE2(v.data, _mm_set1_ps(s));                     \
        else if constexpr (std::is_same_v<T, double> && LD_SSE2)     \
            return DSSE2(v.data, _mm_set1_pd(s));                    \
        else                                                         \
            return TVec4<T>(v.x OP s, v.y OP s, v.z OP s, v.w OP s); \
    }

#define LD_VEC4_ARITH(OP, OP_ASSIGN, SSE2, DSSE2)                                            \
    template <typename T>                                                                    \
    inline TVec4<T> operator OP(const TVec4<T>& lhs, const TVec4<T>& rhs)                    \
    {                                                                                        \
        if constexpr (std::is_same_v<T, float> && LD_SSE2)                                   \
            return SSE2(lhs.data, rhs.data);                                                 \
        else if constexpr (std::is_same_v<T, double> && LD_SSE2)                             \
            return DSSE2(lhs.data, rhs.data);                                                \
        else                                                                                 \
            return TVec4<T>(lhs.x OP rhs.x, lhs.y OP rhs.y, lhs.z OP rhs.z, lhs.w OP rhs.w); \
    }                                                                                        \
    template <typename T>                                                                    \
    inline TVec4<T>& operator OP_ASSIGN(TVec4<T>& lhs, const TVec4<T>& rhs)                  \
    {                                                                                        \
        lhs = lhs OP rhs;                                                                    \
        return lhs;                                                                          \
    }

#define LD_VEC4_UNARY(OP)                                \
    template <typename T>                                \
    inline TVec4<T> operator OP(TVec4<T>& v)             \
    {                                                    \
        return TVec4<T>(OP v.x, OP v.y, OP v.z, OP v.w); \
    }

LD_VEC4_SCALAR(+, _mm_add_ps, _mm_add_pd)
LD_VEC4_SCALAR(-, _mm_sub_ps, _mm_sub_pd)
LD_VEC4_SCALAR(*, _mm_mul_ps, _mm_mul_pd)
LD_VEC4_SCALAR(/, _mm_div_ps, _mm_div_pd)
LD_VEC4_ARITH(+, +=, _mm_add_ps, _mm_add_pd)
LD_VEC4_ARITH(-, -=, _mm_sub_ps, _mm_sub_pd)
LD_VEC4_ARITH(*, *=, _mm_mul_ps, _mm_mul_pd)
LD_VEC4_ARITH(/, /=, _mm_div_ps, _mm_div_pd)
LD_VEC4_UNARY(+)
LD_VEC4_UNARY(-)

#undef LD_VEC4_UNARY
#undef LD_VEC4_ARITH
#undef LD_VEC4_SCALAR

using Vec4 = TVec4<float>;
using IVec4 = TVec4<int>;
using DVec4 = TVec4<double>;

static_assert(LD::IsTrivial<Vec4>);

} // namespace LD
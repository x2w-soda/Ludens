#pragma once

#ifndef LD_MATH_PI
# define LD_MATH_PI     3.141592653589793f
#endif

#ifndef LD_MATH_SQRT
# include <cmath>
# define LD_MATH_SQRT(EXPR)   std::sqrt(EXPR)
#endif

#ifndef LD_MATH_SIN
# include <cmath>
# define LD_MATH_SIN(EXPR)   std::sin(EXPR)
#endif

#ifndef LD_MATH_COS
# include <cmath>
# define LD_MATH_COS(EXPR)   std::cos(EXPR)
#endif

#ifndef LD_MATH_TAN
# include <cmath>
# define LD_MATH_TAN(EXPR)   std::tan(EXPR)
#endif

#ifndef LD_MATH_ASIN
# include <cmath>
# define LD_MATH_ASIN(EXPR)   std::asin(EXPR)
#endif

#ifndef LD_MATH_ACOS
# include <cmath>
# define LD_MATH_ACOS(EXPR)   std::acos(EXPR)
#endif

#ifndef LD_MATH_ATAN
# include <cmath>
# define LD_MATH_ATAN(EXPR)   std::atan(EXPR)
#endif

#ifndef LD_MATH_ABS
# include <cmath>
#define LD_MATH_ABS(EXPR) std::abs(EXPR)
#endif

#ifndef LD_MATH_TOLERANCE
# define LD_MATH_TOLERANCE 1e-5
#endif

#ifndef LD_MATH_EQUAL(LHS, RHS)
# define LD_MATH_EQUAL(LHS, RHS)  (LD_MATH_ABS((LHS) - (RHS)) < LD_MATH_TOLERANCE)
#endif

#ifndef LD_MATH_EQUAL_ZERO(EXPR)
# define LD_MATH_EQUAL_ZERO(EXPR) (LD_MATH_ABS(EXPR) < LD_MATH_TOLERANCE)
#endif

namespace LD {

	template <typename T>
	class TRadians;

	template <typename T>
	class TDegrees
	{
	public:
		TDegrees() = default;
		TDegrees(const T& rotation)
			: Rotation(rotation) {}

		inline TDegrees& operator=(const T& rotation)
		{
			Rotation = rotation;
			return *this;
		}

		inline operator T() const
		{
			return Rotation;
		}

		inline TRadians<T> ToRadians() const
		{
			return Rotation * static_cast<T>(LD_MATH_PI) / static_cast<T>(180);
		}

	private:
		T Rotation;
	};

	template <typename T>
	class TRadians
	{
	public:
		TRadians() = default;
		TRadians(const T& rotation)
			: Rotation(rotation) {}

		inline TRadians& operator=(const T& rotation)
		{
			Rotation = rotation;
			return *this;
		}

		inline operator T() const
		{
			return Rotation;
		}

		inline TDegrees<T> ToDegrees() const
		{
			return Rotation / static_cast<T>(LD_MATH_PI) * static_cast<T>(180);
		}

	private:
		T Rotation;
	};

	using Degrees = TDegrees<float>;
	using Radians = TRadians<float>;

} // namespace LD
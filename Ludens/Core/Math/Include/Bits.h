#pragma once

#include "Core/Header/Include/Types.h"

namespace LD {

	inline u32 CountTrailingZeroBits(u32 x)
	{
		if (x == 0)
			return 32;

		u32 c;

		x = (x ^ (x - 1)) >> 1;
		for (c = 0; x; c++)
			x >>= 1;

		return c;
	}

	inline bool IsPowerOf2(u32 x)
	{
		return (x != 0) && ((x & (x - 1)) == 0);
	}

	inline u32 NextPowerOf2(u32 x)
	{
		if (x == 0)
			return 0;

		--x;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;

		return ++x;
	}

	inline u32 NextPowerOf4(u32 x)
	{
		x = NextPowerOf2(x);
		u32 parity = CountTrailingZeroBits(x) & 1;

		return parity ? (x << 1) : x;
	}

} // namespace LD
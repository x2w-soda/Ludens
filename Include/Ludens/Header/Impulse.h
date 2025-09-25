#pragma once

namespace LD {

/// @brief The impulse class represents a scalar that is reset after read if non-zero.
///        If the scalar is 'truthy' when read, it is reset to a 'falsy' default state. 
template <typename T>
class TImpulse
{
public:
	inline void set(const T& value)
	{
		mValue = value;
	}

	inline T read()
	{
		if (static_cast<bool>(mValue))
		{
			T val = mValue;
			mValue = static_cast<T>(false);
			return val;
		}

		return static_cast<T>(false);
	}

private:
	T mValue;
};

using Impulse = TImpulse<bool>;

} // namespace LD
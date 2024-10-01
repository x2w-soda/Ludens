#pragma once

#include "Core/Header/Include/Error.h"
#include "Core/Header/Include/Types.h"

namespace LD {

	// optional value without pointers or heap allocations
	template <typename T>
	class Optional
	{
	public:
		Optional()
			: mHasValue(false)
		{
		}

		Optional(const T& value)
			: mHasValue(true)
		{
			new (mValue) T(value);
		}

		Optional(const Optional& other)
			: mHasValue(other.mHasValue)
		{
			if (mHasValue)
				new (mValue) T(reinterpret_cast<const T&>(other.mValue));
		}

		~Optional()
		{
			if (mHasValue)
				Get().~T();
		}

		Optional& operator=(const Optional& other)
		{
			if (mHasValue)
				Get().~T();

			if ((mHasValue = other.mHasValue))
				new (mValue) T(reinterpret_cast<const T&>(other.mValue));

			return *this;
		}

		Optional& operator=(const T& value)
		{
			if (mHasValue)
				Get().~T();

			mHasValue = true;
			new (mValue) T(value);

			return *this;
		}

		inline bool HasValue() const
		{
			return mHasValue;
		}

		void Reset()
		{
			if (mHasValue)
				Get().~T();
			mHasValue = false;
		}

		T& Value()
		{
			LD_DEBUG_ASSERT(mHasValue);
			return Get();
		}

		const T& Value() const
		{
			LD_DEBUG_ASSERT(mHasValue);
			return Get();
		}

		const T& ValueOr(const T& fallback) const
		{
			return mHasValue ? Get() : fallback;
		}

		const T* Data() const
		{
			LD_DEBUG_ASSERT(mHasValue);
			return reinterpret_cast<const T*>(mValue);
		}

	private:
		inline T& Get() { return *reinterpret_cast<T*>(mValue); }
		inline const T& Get() const { return *reinterpret_cast<const T*>(mValue); }

		bool mHasValue = false;
		u8 mValue[sizeof(T)];
	};

} // namespace LD
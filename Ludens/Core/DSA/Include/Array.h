#pragma once

#include <algorithm>
#include <initializer_list>
#include "Core/Header/Include/Error.h"
#include "Core/DSA/Include/View.h"


namespace LD {

	template <typename T, size_t TSize>
	class Array
	{
	public:
		Array() = default;
		Array(const Array&) = delete;
		~Array() = default;

		Array(const std::initializer_list<T>& list)
		{
			LD_DEBUG_ASSERT(list.size() == TSize);
			std::copy(list.begin(), list.end(), mData);
		}

		Array& operator=(const Array&) = delete;
		Array& operator=(const std::initializer_list<T>& list)
		{
			LD_DEBUG_ASSERT(list.size() == TSize);
			std::copy(list.begin(), list.end(), mData);
			return *this;
		}

		inline T* Data() { return mData; }
		inline const T* Data() const { return mData; }
		inline constexpr size_t Size() const { return TSize; }
		inline constexpr size_t ByteSize() const { return sizeof(mData); }
		inline const T* Begin() const { return mData; }
		inline const T* End() const { return mData + TSize; }
		inline T* Begin() { return mData; }
		inline T* End() { return mData + TSize; }

		// STL backwards support
		inline const T* begin() const { return Begin(); }
		inline const T* end() const { return End(); }
		inline T* begin() { return Begin(); }
		inline T* end() { return End(); }

		inline View<T> GetView() const
		{
			return View<T> { TSize, mData };
		}

		inline T& operator[](size_t index)
		{
			LD_DEBUG_ASSERT(index < TSize);
			return mData[index];
		}

		inline const T& operator[](size_t index) const
		{
			LD_DEBUG_ASSERT(index < TSize);
			return mData[index];
		}

	private:
		T mData[TSize];
	};

} // namespace LD

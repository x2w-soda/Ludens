#pragma once

#include <cstddef>

namespace LD {

template <typename T>
struct TRange
{
    T offset;
    T size;

    /// @brief Construct range from two endpoint offsets.
    static TRange from_offsets(T offset1, T offset2)
    {
        if (offset1 > offset2)
        {
            T tmp = offset1;
            offset1 = offset2;
            offset2 = tmp;
        }

        return TRange(offset1, offset2 - offset1);
    }

    /// @brief Construct range by clamping against some size.
    static TRange clamp_size(TRange range, size_t clampSize)
    {
        if (clampSize == 0)
            return TRange((T)0, (T)0);

        if (range.offset >= clampSize)
            return TRange(clampSize, (T)0);

        if (range.offset + range.size >= clampSize)
            return TRange(range.offset, clampSize - range.offset);

        return range;
    }

    inline operator bool() const noexcept
    {
        return size > 0;
    }

    inline bool operator==(const TRange& other) const noexcept
    {
        return offset == other.offset && size == other.size;
    }
};

using Range = TRange<size_t>;

} // namespace LD
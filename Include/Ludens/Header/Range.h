#pragma once

#include <cstddef>

namespace LD {

template <typename T>
struct TRange
{
    T offset;
    T size;

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
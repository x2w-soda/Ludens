#pragma once

#include <cstddef>

namespace LD {

template <typename T>
struct TRange
{
    T offset;
    T size;
};

using Range = TRange<size_t>;

} // namespace LD
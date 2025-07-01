#pragma once

#include <type_traits>

namespace LD {

using byte = unsigned char;

template <typename T>
concept IsTrivial = std::is_trivially_constructible_v<T> &&
                    std::is_trivially_destructible_v<T> &&
                    std::is_trivially_copyable_v<T>;

} // namespace LD
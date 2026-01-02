#pragma once

#include <unordered_map>

namespace LD {

template <typename TKey, typename TVal>
using HashMap = std::unordered_map<TKey, TVal>;

} // namespace LD
#pragma once

#include <cstddef>

namespace LD
{

template <typename TClass>
struct PtrHash
{
    std::size_t operator()(const TClass* object) const
    {
        return std::hash<const void*>{}(object);
    }
};

template <typename TClass>
struct PtrEqual
{
    bool operator()(const TClass* lhs, const TClass* rhs) const
    {
        return lhs == rhs;
    }
};

} // namespace LD
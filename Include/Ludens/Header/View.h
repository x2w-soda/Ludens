#pragma once

#include <cstddef>
#include <cstring>
#include <format>

namespace LD {

/// @brief View into sequential data.
template <typename T>
struct TView
{
    T* data;
    size_t size;

    /// @brief A view is 'truthy' if and only if it is non-null and non-zero size.
    inline operator bool() const { return data && size > 0; }

    /// @brief A view is equal to a C string if and only if they have same byte size and contents.
    ///        Returns false if view is 'falsy' or string is null.
    bool operator==(const char* cstr) const
    {
        if (!*this || !cstr)
            return false;

        size_t len = strlen(cstr);
        if (size != len)
            return false;

        for (size_t i = 0; i < len; i++)
        {
            if (static_cast<T>(cstr[i]) != data[i])
                return false;
        }

        return true;
    }

    /// @brief Two views are equal if and only if they have same byte size and contents.
    ///        Returns false if either view is 'falsy'.
    bool operator==(const TView& other) const
    {
        if (!*this || !other || size != other.size)
            return false;

        if (data == other.data)
            return true;

        for (size_t i = 0; i < size; i++)
        {
            if (data[i] != other.data[i])
                return false;
        }

        return true;
    }
};

/// @brief Const view of a byte sequence.
using View = TView<const char>;

} // namespace LD

template <>
struct std::formatter<LD::View, char>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const LD::View& view, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", std::string(view.data, view.size));
    }
};
#pragma once

#include <cstddef>
#include <cstring>
#include <format>
#include <string>
#include <type_traits>

namespace LD {

/// @brief View into sequential data.
template <typename T>
struct TView
{
    T* data;
    size_t size;

    TView() = default;

    TView(T* data, size_t size)
        : data(data), size(size)
    {
    }

    /// @brief View into C string.
    TView(const char* cstr)
        : data(cstr)
    {
        static_assert(sizeof(T) == 1);
        size = cstr ? strlen(cstr) : 0;
    }

    /// @brief View into std string, valid before the std string invalidates.
    TView(const std::string& str)
        : data((T*)str.data()), size(str.size())
    {
        static_assert(sizeof(T) == 1);
    }

    /// @brief Copy std string_view, valid before the std string_view invalidates.
    TView(const std::string_view& strView)
        : data(strView.data()), size(strView.size())
    {
        static_assert(sizeof(T) == 1);
    }

    /// @brief A view is 'truthy' if and only if it is non-null and non-zero size.
    inline operator bool() const { return data && size > 0; }

    /// @brief A view is equal to a C string if and only if they have same byte size and contents.
    ///        Returns false if input C string is null.
    bool operator==(const char* cstr) const
    {
        if (!cstr)
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

/// @brief Mutable view of a byte sequence.
using MutView = TView<char>;

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
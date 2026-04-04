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
    requires std::is_same_v<T, char> || std::is_same_v<T, const char>
        : data(cstr)
    {
        size = cstr ? strlen(cstr) : 0;
    }

    /// @brief View into std string, valid before the std string invalidates.
    TView(const std::string& str)
    requires std::is_same_v<T, char> || std::is_same_v<T, const char>
        : data((T*)str.data()), size(str.size())
    {
    }

    /// @brief Copy std string_view, valid before the std string_view invalidates.
    TView(const std::string_view& strView)
    requires std::is_same_v<T, char> || std::is_same_v<T, const char>
        : data(strView.data()), size(strView.size())
    {
    }

    /// @brief A view is 'truthy' iff it is non-null and non-zero size.
    inline operator bool() const { return data && size > 0; }

    /// @brief A view is equal to a C string iff they have same byte size and contents.
    ///        Returns false if input C string is null.
    bool operator==(const char* cstr) const
    requires std::is_same_v<T, char> || std::is_same_v<T, const char>
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

    /// @brief A view is equal to a char iff the view is of size one and the character matches.
    inline bool operator==(char c) const
    requires std::is_same_v<T, char> || std::is_same_v<T, const char>
    {
        return size == 1 && static_cast<char>(data[0]) == c;
    }

    /// @brief Two views are equal iff they have same byte size and contents.
    ///        Returns false if either view is 'falsy'.
    bool operator==(const TView& other) const
    {
        if (!*this || !other || size != other.size)
            return false;

        if (data == other.data)
            return true;

        return !memcmp(data, other.data, sizeof(T) * size);
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
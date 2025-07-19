#pragma once

#include <cstddef>
#include <cstring>

namespace LD {

/// @brief A non-owning, read-only view of a sequence of characters
/// @tparam T character type
template <typename T>
class TStringView
{
public:
    TStringView() = default;

    TStringView(const T* data, size_t size)
        : mData(data), mSize(size)
    {
    }

    inline const T* data() const
    {
        return mData;
    }

    inline size_t size() const
    {
        return mSize;
    }

    /// @brief compare string view to null-terminated C string
    bool operator==(const char* cstr) const
    {
        size_t len = strlen(cstr);

        if (mSize != len)
            return false;

        for (size_t i = 0; i < len; i++)
        {
            if (cstr[i] != (char)mData[i])
                return false;
        }

        return true;
    }

    /// @brief string views are equal if they have the same length and
    ///        their character sequences are identical.
    bool operator==(const TStringView& other) const
    {
        if (mData == other.mData)
            return mSize == other.mSize;

        return mSize == other.mSize && !strncmp(mData, other.mData, mSize);
    }

    inline bool operator!=(const TStringView& other) const
    {
        return !this->operator==(other);
    }

private:
    const T* mData;
    size_t mSize;
};

using StringView = TStringView<char>;

} // namespace LD
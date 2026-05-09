#pragma once

#include <Ludens/Header/Hash.h>
#include <Ludens/Header/Types.h>
#include <Ludens/Header/View.h>
#include <Ludens/Memory/Memory.h>

#include <algorithm> // std::min
#include <cstring>
#include <format>
#include <ostream>

// This is a lot of headroom for SSO local storage.
#define STRING_DEFAULT_LOCAL_SIZE 32

namespace LD {

/// @brief String class with template SSO parameter.
/// @tparam TLocalSize local storage
/// @tparam TUsage usage hint
template <size_t TLocalSize, MemoryUsage TUsage = MEMORY_USAGE_MISC>
class TString
{
public:
    TString()
        : mBase(mLocal)
    {
        mBase[0] = '\0';
    }

    TString(const char* cstr)
        : mBase(mLocal)
    {
        if (!cstr)
        {
            mSize = 0;
            mBase[0] = '\0';
            return;
        }

        resize(strlen(cstr));
        memcpy(mBase, cstr, mSize);
    }

    TString(const char8_t* buf, size_t len)
        : mBase(mLocal)
    {
        resize(len);
        memcpy(mBase, buf, len);
    }

    TString(const byte* buf, size_t len)
        : mBase(mLocal)
    {
        resize(len);
        memcpy(mBase, buf, len);
    }

    TString(const char* str, size_t len)
        : mBase(mLocal)
    {
        resize(len);
        memcpy(mBase, str, len);
    }

    TString(View view)
        : mBase(mLocal)
    {
        resize(view.size);
        memcpy(mBase, view.data, view.size);
    }

    TString(const TString& other)
        : mBase(mLocal)
    {
        resize(other.mSize);
        memcpy(mBase, other.mBase, mSize);
    }

    TString(TString&& other) noexcept
        : mBase(mLocal), mSize(other.mSize), mCap(other.mCap)
    {
        if (other.mBase == other.mLocal)
            memcpy(mLocal, other.mLocal, other.mSize);
        else
        {
            mBase = other.mBase;
            other.mBase = other.mLocal;
        }

        mBase[mSize] = '\0';

        other.mSize = 0;
        other.mCap = 0;
    }

    ~TString()
    {
        if (mBase != mLocal)
            heap_free(mBase);
    }

    TString& operator=(const TString& other)
    {
        resize(other.mSize);
        memcpy(mBase, other.mBase, other.mSize);

        return *this;
    }

    TString& operator=(TString&& other) noexcept
    {
        if (*this != other)
        {
            mBase = mLocal;
            mSize = other.mSize;
            mCap = other.mSize;

            if (other.mBase == other.mLocal)
                memcpy(mLocal, other.mLocal, other.mSize);
            else
            {
                mBase = other.mBase;
                other.mBase = other.mLocal;
            }

            mBase[mSize] = '\0';
        }

        other.mSize = 0;
        other.mCap = 0;

        return *this;
    }

    inline size_t size() const
    {
        return mSize;
    }

    inline size_t capacity() const
    {
        return mCap;
    }

    inline bool empty() const
    {
        return mSize == 0;
    }

    void clear()
    {
        resize(0);
    }

    const char8_t* data() const
    {
        return mBase;
    }

    char8_t* data()
    {
        return mBase;
    }

    /// @brief Adjust string size
    /// @param nsize new size
    void resize(size_t nsize)
    {
        if (nsize + 1 <= mCap)
        {
            mSize = nsize;
            mBase[mSize] = '\0';
            return;
        }

        if (mBase == mLocal) // migrate to heap storage
        {
            mBase = (char8_t*)heap_malloc(nsize + 1, TUsage);
            memcpy(mBase, mLocal, mSize);
            mBase[nsize] = '\0';
        }
        else // resize heap storage
        {
            char8_t* nbase = (char8_t*)heap_malloc(nsize + 1, TUsage);
            memcpy(nbase, mBase, mSize);
            heap_free(mBase);
            mBase = nbase;
            mBase[nsize] = '\0';
        }

        mSize = nsize;
        mCap = nsize;
    }

    /// @brief replace a portion of string
    /// @param pos the starting position of the replacement
    /// @param len the length of the portion to be replaced
    /// @param rep the new string value to replace with
    /// @param rlen the length of the new replacement string
    void replace(size_t pos, size_t len, const void* rep, size_t rlen)
    {
        if (rlen >= len)
        {
            size_t shift = rlen - len;
            resize(mSize + shift);
            size_t last = mSize - 1;

            for (size_t i = 0; (pos + rlen + i) < mSize; i++)
                mBase[last - i] = mBase[last - shift - i];

            for (size_t i = 0; i < rlen; i++)
                mBase[pos + i] = ((const byte*)rep)[i];
        }
        else
        {
            size_t shift = len - rlen;
            size_t first = pos + rlen;

            for (size_t i = 0; (first + shift + i) < mSize; i++)
                mBase[first + i] = mBase[first + shift + i];

            for (size_t i = 0; i < rlen; i++)
                mBase[pos + i] = ((const byte*)rep)[i];

            resize(mSize - shift);
        }
    }

    /// @brief get a substring
    /// @param pos starting position
    /// @param len substring span
    /// @return a new string from substring
    TString substr(size_t pos, size_t len) const
    {
        return TString(mBase + pos, len);
    }

    /// @brief insert a string at position
    /// @param pos position to insert string
    /// @param str source string to insert
    /// @param len length of source string
    inline void insert(size_t pos, const char8_t* str, size_t len)
    {
        replace(pos, 0, str, len);
    }

    /// @brief append a string at the back
    /// @param str source string to append
    /// @param len length of source string
    inline void append(const char8_t* str, size_t len)
    {
        replace(mSize, 0, str, len);
    }

    inline void append(View view)
    {
        append((const char8_t*)view.data, view.size);
    }

    void erase(size_t pos, size_t size)
    {
        if (pos >= mSize)
            return;

        size = std::min(size, mSize - pos);

        for (size_t i = pos; i + size < mSize; i++)
            mBase[i] = mBase[i + size];

        resize(mSize - size);
    }

    size_t find(const void* data, size_t size)
    {
        if (size == 0)
            return 0; // match at start

        if (size > mSize)
            return npos;

        for (size_t i = 0; i <= mSize - size; i++)
        {
            if (!memcmp(mBase + i, data, size))
                return i;
        }

        return npos;
    }

    inline size_t find(const char* cstr)
    {
        return cstr ? find(cstr, strlen(cstr)) : npos;
    }

    inline size_t find(char c)
    {
        return find(&c, 1);
    }

    inline char8_t& back() noexcept
    {
        return mBase[mSize - 1];
    }

    inline char8_t back() const noexcept
    {
        return mBase[mSize - 1];
    }

    inline void push_back(char c) noexcept
    {
        resize(mSize + 1);
        mBase[mSize - 1] = (char8_t)c;
    }

    inline void pop_back() noexcept
    {
        resize(mSize - 1);
    }

    inline const char* c_str() const noexcept
    {
        return (const char*)mBase;
    }

    inline operator View() const noexcept
    {
        return View((const byte*)mBase, mSize);
    }

    /// @brief Copy from C string.
    void operator=(const char* cstr)
    {
        if (!cstr)
        {
            resize(0);
            return;
        }

        resize(strlen(cstr));
        memcpy(mBase, cstr, mSize);
    }

    /// @brief Compare with View.
    bool operator==(View view) const
    {
        if (mSize != view.size)
            return false;

        return !memcmp(mBase, view.data, view.size);
    }

    /// @brief Compare with C string.
    bool operator==(const char* cstr) const
    {
        if (mSize != strlen(cstr))
            return false;

        return !memcmp(mBase, cstr, mSize);
    }

    /// @brief Compare with another string.
    bool operator==(const TString& other) const
    {
        if (mSize != other.mSize)
            return false;

        return !memcmp(mBase, other.mBase, mSize);
    }

    inline char8_t operator[](int index) const noexcept
    {
        return mBase[index];
    }

    inline TString& operator=(View view)
    {
        resize(view.size);
        memcpy(mBase, view.data, view.size);
        return *this;
    }

    inline TString& operator+=(View view)
    {
        append(view);
        return *this;
    }

    inline TString& operator+=(const TString& other)
    {
        append(other.data(), other.size());
        return *this;
    }

    inline TString& operator+=(const char* cstr)
    {
        if (cstr)
            append((const char8_t*)cstr, strlen(cstr));
        return *this;
    }

public: // static
    static constexpr size_t npos{static_cast<size_t>(-1)};

private:
    char8_t* mBase;
    size_t mSize = 0;
    size_t mCap = TLocalSize;
    char8_t mLocal[TLocalSize];
};

using String = TString<STRING_DEFAULT_LOCAL_SIZE, MEMORY_USAGE_MISC>;

} // namespace LD

template <size_t TLocalSize, LD::MemoryUsage TUsage>
struct std::formatter<LD::TString<TLocalSize, TUsage>> : std::formatter<std::string_view>
{
    auto format(const LD::TString<TLocalSize, TUsage>& str, std::format_context& ctx) const
    {
        return std::formatter<std::string_view>::format(std::string_view((const char*)str.data(), str.size()), ctx);
    }
};

template <size_t TLocalSize, LD::MemoryUsage TUsage>
struct std::hash<LD::TString<TLocalSize, TUsage>>
{
    size_t operator()(const LD::TString<TLocalSize, TUsage>& str) const noexcept
    {
        return (size_t)LD::hash64_FNV_1a((const char*)str.data(), str.size());
    }
};
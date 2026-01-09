#pragma once

#include <Ludens/DSA/HeapStorage.h>
#include <Ludens/Memory/Memory.h>
#include <cstring>

#define STRING_DEFAULT_LOCAL_STORAGE 12

namespace LD {

/// @brief string class with SSO
/// @tparam T character type
/// @tparam TLocalSize local storage
/// @tparam TUsage usage hint
template <typename T, size_t TLocalSize, MemoryUsage TUsage = MEMORY_USAGE_MISC>
class TString
{
public:
    TString()
        : mBase(mLocal)
    {
        mHeap.cap = TLocalSize;
    }

    TString(const char* cstr)
        : mBase(mLocal)
    {
        mHeap.cap = TLocalSize;
        size_t len = strlen(cstr);
        resize(len);

        if constexpr (sizeof(T) == 1)
        {
            memcpy(mBase, cstr, len);
        }
        else
        {
            for (size_t i = 0; i < len; i++)
                mBase[i] = static_cast<T>(cstr[i]);
        }
    }

    TString(const T* str, size_t len)
        : mBase(mLocal)
    {
        mHeap.cap = TLocalSize;
        resize(len);
        memcpy(mBase, str, len * sizeof(T));
    }

    TString(const TString& other)
        : mBase(mLocal)
    {
        mHeap.cap = TLocalSize;
        resize(other.mHeap.size);
        memcpy(mBase, other.mBase, other.mHeap.size * sizeof(T));
    }

    TString(TString&& other)
        : mBase(mLocal)
    {
        mHeap.size = other.mHeap.size;
        mHeap.cap = other.mHeap.cap;

        if (other.mBase == other.mLocal)
        {
            // copy local data over
            memcpy(mLocal, other.mLocal, other.mHeap.size * sizeof(T));
        }
        else
        {
            // move container over
            mHeap = std::move(other.mHeap);
            mBase = mHeap.data;
        }

        other.mHeap.size = 0;
        other.mHeap.cap = 0;
    }

    ~TString()
    {
        if (mBase == mHeap.data)
            mHeap.release();
    }

    TString& operator=(const TString& other)
    {
        resize(other.mHeap.size);
        memcpy(mBase, other.mBase, other.mHeap.size * sizeof(T));

        return *this;
    }

    TString& operator=(TString&& other)
    {
        mBase = mLocal;
        mHeap.size = other.mHeap.size;
        mHeap.cap = other.mHeap.cap;

        if (other.mBase == other.mLocal)
        {
            // copy local data over
            memcpy(mLocal, other.mLocal, other.mHeap.size * sizeof(T));
        }
        else
        {
            // move container over
            mHeap = std::move(other.mHeap);
            mBase = mHeap.data;
        }

        other.mHeap.size = 0;
        other.mHeap.cap = 0;

        return *this;
    }

    size_t size() const
    {
        return mHeap.size;
    }

    size_t capacity() const
    {
        return mHeap.cap;
    }

    bool empty() const
    {
        return mHeap.size == 0;
    }

    void clear()
    {
        resize(0);
    }

    const T* data() const
    {
        return mBase;
    }

    T* data()
    {
        return mBase;
    }

    /// @brief adjust string size
    /// @param nsize new size
    void resize(size_t nsize)
    {
        if (nsize <= mHeap.cap)
        {
            mHeap.size = nsize;
            return;
        }

        if (mBase == mLocal)
        {
            size_t size = mHeap.size;

            // migrate to heap storage
            mHeap.cap = 0;
            mHeap.size = 0;
            mHeap.resize(nsize);

            memcpy(mHeap.data, mLocal, sizeof(T) * size);
        }
        else
        {
            // resize heap storage
            mHeap.resize(nsize);
        }

        mBase = mHeap.data;
    }

    /// @brief replace a portion of string
    /// @param pos the starting position of the replacement
    /// @param len the length of the portion to be replaced
    /// @param rep the new string value to replace with
    /// @param rlen the length of the new replacement string
    void replace(size_t pos, size_t len, const T* rep, size_t rlen)
    {
        if (rlen >= len)
        {
            size_t shift = rlen - len;
            resize(mHeap.size + shift);
            size_t last = mHeap.size - 1;

            for (size_t i = 0; (pos + rlen + i) < mHeap.size; i++)
                mBase[last - i] = mBase[last - shift - i];

            for (size_t i = 0; i < rlen; i++)
                mBase[pos + i] = rep[i];
        }
        else
        {
            size_t shift = len - rlen;
            size_t first = pos + rlen;

            for (size_t i = 0; (first + shift + i) < mHeap.size; i++)
                mBase[first + i] = mBase[first + shift + i];

            for (size_t i = 0; i < rlen; i++)
                mBase[pos + i] = rep[i];

            resize(mHeap.size - shift);
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
    inline void insert(size_t pos, const T* str, size_t len)
    {
        replace(pos, 0, str, len);
    }

    /// @brief append a string at the back
    /// @param str source string to append
    /// @param len length of source string
    inline void append(const T* str, size_t len)
    {
        replace(mHeap.size, 0, str, len);
    }

    void operator=(const char* cstr)
    {
        if (!cstr)
        {
            resize(0);
            return;
        }

        size_t len = strlen(cstr);

        resize(len);

        if constexpr (sizeof(T) == 1)
        {
            memcpy(mBase, cstr, len);
        }
        else
        {
            for (size_t i = 0; i < len; i++)
                mBase[i] = static_cast<T>(cstr[i]);
        }
    }

    /// @brief compare with null terminated c string, one character at a time
    bool operator==(const char* cstr) const
    {
        size_t len = strlen(cstr);
        if (mHeap.size != len)
            return false;

        for (size_t i = 0; i < len; i++)
        {
            if (mBase[i] != (T)cstr[i])
                return false;
        }

        return true;
    }

    /// @brief compare with another string, one character at a time
    bool operator==(const TString& other) const
    {
        if (mHeap.size != other.mHeap.size)
            return false;

        for (size_t i = 0; i < mHeap.size; i++)
        {
            if (mBase[i] != other.mBase[i])
                return false;
        }

        return true;
    }

private:
    T* mBase;
    T mLocal[TLocalSize];
    THeapStorage<T, TUsage> mHeap;
};

/// @brief String type with single-byte characters.
using String = TString<char, STRING_DEFAULT_LOCAL_STORAGE, MEMORY_USAGE_MISC>;

} // namespace LD
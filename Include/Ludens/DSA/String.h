#pragma once

#include <Ludens/System/Memory.h>
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

    void operator=(const char* cstr)
    {
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
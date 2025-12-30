#pragma once

#include <Ludens/DSA/HeapStorage.h>
#include <Ludens/System/Memory.h>

#include <utility>
#include <vector>

namespace LD {

template <typename T>
using Vector = std::vector<T>;

#if 0
/// @brief small vector optimization via local storage
/// @tparam T element type, must be default-constructable
/// @tparam TLocalSize local array size
/// @tparam TUsage memory usage hint
/// @note This class may be used to reduce heap allocations when
///       the expected element count is known. Heap allocations
///       only occur when the local storage is not enough.
template <typename T, size_t TLocalSize, MemoryUsage TUsage = MEMORY_USAGE_MISC>
class SVector
{
public:
    SVector()
        : mBase(mLocal)
    {
        mHeap.cap = TLocalSize;
    }

    SVector(size_t size)
        : mBase(mLocal)
    {
        mHeap.cap = TLocalSize;
        resize(size);
    }

    SVector(size_t size, const T& value)
        : mBase(mLocal)
    {
        mHeap.cap = TLocalSize;
        resize(size);
        for (size_t i = 0; i < size; i++)
            mBase[i] = value;
    }

    SVector(const SVector& other)
        : mBase(mLocal)
    {
        mHeap.cap = TLocalSize;
        resize(other.size());
        for (size_t i = 0; i < mHeap.size; i++)
            mBase[i] = other.mBase[i];
    }

    SVector(SVector&& other)
        : mBase(mLocal)
    {
        // cap and size always tagged in THeapStorage, whether local or not
        mHeap.cap = other.mHeap.cap;
        mHeap.size = other.mHeap.size;

        if (other.mBase == other.mLocal)
        {
            // move local array over
            for (size_t i = 0; i < mHeap.size; i++)
                mLocal[i] = std::move(other.mLocal[i]);
        }
        else
        {
            // move heap storage over
            mHeap = std::move(other.mHeap);
            mBase = mHeap.data;
        }

        other.mHeap.cap = 0;
        other.mHeap.size = 0;
    }

    ~SVector()
    {
        if (mBase == mHeap.data)
            mHeap.release();
    }

    SVector& operator=(const SVector& other)
    {
        resize(other.size());

        for (size_t i = 0; i < mHeap.size; i++)
            mBase[i] = other.mBase[i];

        return *this;
    }

    SVector& operator=(SVector&& other)
    {
        // cap and size always tagged in THeapStorage, whether local or not
        mHeap.cap = other.mHeap.cap;
        mHeap.size = other.mHeap.size;

        if (other.mBase == other.mLocal)
        {
            // move local array over
            for (size_t i = 0; i < mHeap.size; i++)
                mLocal[i] = std::move(other.mLocal[i]);
            mBase = mLocal;
        }
        else
        {
            // move heap storage over
            mHeap = std::move(other.mHeap);
            mBase = mHeap.data;
        }

        other.mHeap.cap = 0;
        other.mHeap.size = 0;

        return *this;
    }

    inline const T& operator[](int idx) const
    {
        return mBase[idx];
    }

    inline T& operator[](int idx)
    {
        return mBase[idx];
    }

    inline size_t size() const
    {
        return mHeap.size;
    }

    inline size_t capacity() const
    {
        return mHeap.cap;
    }

    inline bool empty() const
    {
        return mHeap.size == 0;
    }

    inline void clear()
    {
        resize(0);
    }

    inline const T* data() const
    {
        return mBase;
    }

    inline T* data()
    {
        return mBase;
    }

    /// @brief adjust vector size
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

            for (size_t i = 0; i < size; i++)
                mHeap.data[i] = std::move(mLocal[i]);
        }
        else
        {
            // resize heap storage
            mHeap.resize(nsize);
        }

        mBase = mHeap.data;
    }

    void push_back(const T& other)
    {
        resize(mHeap.size + 1);
        mBase[mHeap.size - 1] = other;
    }

    void push_back(T&& other)
    {
        resize(mHeap.size + 1);
        mBase[mHeap.size - 1] = std::move(other);
    }

    template <typename... TArgs>
    void emplace_back(TArgs&&... args)
    {
        resize(mHeap.size + 1);
        new (mBase + mHeap.size - 1) T(std::forward<TArgs>(args)...);
    }

private:
    T* mBase;
    T mLocal[TLocalSize];
    THeapStorage<T, TUsage> mHeap;
};
#endif

} // namespace LD
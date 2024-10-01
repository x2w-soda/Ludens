#pragma once

#include <cstring>
#include <initializer_list>
#include "Core/Header/Include/Error.h"
#include "Core/Math/Include/Bits.h"
#include "Core/DSA/Include/View.h"
#include "Core/DSA/Include/Buffer.h"
#include "Core/OS/Include/Memory.h"
#include "Core/OS/Include/Allocator.h"

namespace LD {

template <typename T>
class VectorBase
{
public:
    VectorBase() : mData(nullptr), mSize(0)
    {
    }

    // derived class implements Resize such that
    // mSize is set to parameter size
    // mData can be indexed by indices in the range [0, size)
    virtual void Resize(size_t size) = 0;

    inline T* Data()
    {
        return mData;
    }

    inline const T* Data() const
    {
        return mData;
    }

    inline bool IsEmpty() const
    {
        return mSize == 0;
    }

    inline size_t Size() const
    {
        return mSize;
    }

    inline size_t ByteSize() const
    {
        return sizeof(T) * mSize;
    }

    inline void PushBack(const T& item)
    {
        Resize(mSize + 1);
        mData[mSize - 1] = item;
    }

    inline void PushBack(T&& item)
    {
        Resize(mSize + 1);
        mData[mSize - 1] = std::move(item);
    }

    inline T& PushBack()
    {
        Resize(mSize + 1);
        mData[mSize - 1] = {};

        return mData[mSize - 1];
    }

    inline void PopBack()
    {
        if (mSize == 0)
            return;

        Resize(mSize - 1);
    }

    inline void Clear()
    {
        Resize(0);
    }

    inline T& Front()
    {
        LD_DEBUG_ASSERT(mSize > 0);
        return mData[0];
    }

    inline const T& Front() const
    {
        LD_DEBUG_ASSERT(mSize > 0);
        return mData[0];
    }

    inline T& Back()
    {
        LD_DEBUG_ASSERT(mSize > 0);
        return mData[mSize - 1];
    }

    inline const T& Back() const
    {
        LD_DEBUG_ASSERT(mSize > 0);
        return mData[mSize - 1];
    }

    inline const T* Begin() const
    {
        return mData;
    }

    inline const T* End() const
    {
        return mData + mSize;
    }

    inline T* Begin()
    {
        return mData;
    }

    inline T* End()
    {
        return mData + mSize;
    }

    // STL backwards support
    inline const T* begin() const
    {
        return Begin();
    }

    inline const T* end() const
    {
        return End();
    }

    inline T* begin()
    {
        return Begin();
    }

    inline T* end()
    {
        return End();
    }

    inline View<T> GetView()
    {
        return View<T>{mSize, mData};
    }

    inline T& operator[](size_t index)
    {
        LD_DEBUG_ASSERT(index < mSize);
        return mData[index];
    }

    inline const T& operator[](size_t index) const
    {
        LD_DEBUG_ASSERT(index < mSize);
        return mData[index];
    }

protected:
    size_t mSize = 0;
    T* mData = nullptr;
};

template <typename T>
class Vector : public VectorBase<T>
{
public:
    Vector() = default;

    Vector(size_t size)
    {
        Resize(size);
    }

    Vector(const Vector<T>& other)
    {
        Resize(other.mSize);
        std::copy(other.Begin(), other.End(), Begin());
    }

    Vector(const std::initializer_list<T>& list)
    {
        Resize(list.size());
        std::copy(list.begin(), list.end(), Begin());
    }

    ~Vector()
    {
        Resize(0);
    }

    Vector<T>& operator=(const Vector<T>& other)
    {
        Resize(other.mSize);
        std::copy(other.Begin(), other.End(), Begin());
        return *this;
    }

    virtual void Resize(size_t size) override
    {
        if (size <= mSize)
        {
            for (size_t i = size; i < mSize; i++)
            {
                mData[i].~T();
            }
            mSize = size;
            return;
        }

        mBuffer.Resize(sizeof(T) * size);
        mData = (T*)mBuffer.Data();

        for (size_t i = mSize; i < size; i++)
        {
            new (mData + i) T();
        }

        mSize = size;
    }

private:
    Buffer mBuffer;
};

template <typename T, size_t TLocalSize>
class SmallVector : public VectorBase<T>
{
public:
    SmallVector()
    {
        mData = reinterpret_cast<T*>(mLocal);
        mSize = 0;
    }

    SmallVector(size_t size)
    {
        Resize(size);
    }

    SmallVector(const std::initializer_list<T>& list)
    {
        Resize(list.size());
        std::copy(list.begin(), list.end(), Begin());
    }

    SmallVector(const SmallVector& other)
    {
        Resize(other.Size());
        std::copy(other.Begin(), other.End(), Begin());
    }

    ~SmallVector()
    {
        Resize(0);
    }

    SmallVector& operator=(const SmallVector& other)
    {
        Resize(other.Size());
        std::copy(other.Begin(), other.End(), Begin());
        return *this;
    }

    virtual void Resize(size_t size) override
    {
        if (size <= mSize)
        {
            for (size_t i = size; i < mSize; i++)
            {
                mData[i].~T();
            }
            mSize = size;
            return;
        }

        if (mData == reinterpret_cast<T*>(mLocal))
        {
            if (size > TLocalSize)
            {
                // once we exceed local array size, we start allocating heap memory,
                // the local array is never used again even if we shrink later.
                mBuffer.Resize(sizeof(T) * size);
                mData = (T*)mBuffer.Data();
                memcpy(mData, mLocal, sizeof(T) * mSize);
            }
        }
        else
        {
            mBuffer.Resize(sizeof(T) * size);
            mData = (T*)mBuffer.Data();
        }

        for (size_t i = mSize; i < size; i++)
        {
            new (mData + i) T();
        }

        mSize = size;
    }

private:
    Buffer mBuffer;
    char* mLocal[sizeof(T) * TLocalSize];
};

} // namespace LD
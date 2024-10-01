#pragma once

#include <algorithm>
#include "Core/DSA/Include/Buffer.h"

namespace LD {

template <typename T>
class Stack
{
public:
    Stack() = default;

    Stack(const Stack<T>& other)
    {
        Resize(other.mSize);
        std::copy(other.mData, other.mData + mSize, mData);
    }

    ~Stack()
    {
        Resize(0);
    }

    Stack<T>& operator=(const Stack<T>& other)
    {
        Resize(other.mSize);
        std::copy(other.mData, other.mData + mSize, mData);
        return *this;
    }

    void Push(const T& value)
    {
        Resize(mSize + 1);
        mData[mSize - 1] = value;
    }

    /// default constructs an element in the container and returns its reference
    T& Push()
    {
        Resize(mSize + 1);
        return mData[mSize - 1];
    }

    /// remove top element
    void Pop()
    {
        LD_DEBUG_ASSERT(mSize > 0);
        Resize(mSize - 1);
    }

    const T& Top() const
    {
        LD_DEBUG_ASSERT(mSize > 0);
        return mData[mSize - 1];
    }

    T& Top()
    {
        LD_DEBUG_ASSERT(mSize > 0);
        return mData[mSize - 1];
    }

    inline size_t Size() const
    {
        return mSize;
    }

    inline bool IsEmpty()
    {
        return mSize == 0;
    }

    /// pops all elements
    void Clear()
    {
        Resize(0);
    }

private:
    void Resize(size_t size)
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

    Buffer mBuffer;
    size_t mSize = 0;
    T* mData = nullptr;
};

} // namespace LD
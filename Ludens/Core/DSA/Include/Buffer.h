#pragma once

#include "Core/OS/Include/Memory.h"
#include "Core/Math/Include/Bits.h"

namespace LD {

/// contiguous memory on the heap, copies old data to new address when resized
class Buffer
{
public:
    Buffer() = default;
    Buffer(size_t size)
    {
        Resize(size);
    }
    Buffer(const Buffer&) = delete;
    ~Buffer()
    {
        if (mData)
        {
            MemoryFree(mData);
            mData = nullptr;
            mSize = 0;
            mAllocSize = 0;
        }
    }

    Buffer& operator=(const Buffer&) = delete;

    /// resize the buffer, old data will be copied over to new address
    void Resize(size_t size)
    {
        size_t newAllocSize = (size_t)NextPowerOf2((u32)size);

        // shrinking or growing but does not trigger realloc
        if (size <= mSize || newAllocSize <= mAllocSize)
        {
            mSize = size;
            return;
        }

        mSize = size;
        mAllocSize = newAllocSize;
        mData = MemoryRealloc(mData, newAllocSize);
    }

    /// get the byte size of the buffer
    inline size_t Size() const
    {
        return mSize;
    }

    /// get the actual allocated byte size of the buffer
    inline size_t AllocSize() const
    {
        return mAllocSize;
    }

    /// get the base address of const buffer data, may be null if size is zero
    inline const void* Data() const
    {
        return mData;
    }

    /// get the base address of buffer data, may be null if size is zero
    inline void* Data()
    {
        return mData;
    }

private:
    void* mData = nullptr;
    size_t mSize = 0;
    size_t mAllocSize = 0;
};

} // namespace LD
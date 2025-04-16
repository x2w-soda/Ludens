#pragma once

#include <Ludens/System/Memory.h>

namespace LD {

class LinearAllocator
{
public:
    LinearAllocator();
    LinearAllocator(LinearAllocator&) = delete;
    ~LinearAllocator();

    LinearAllocator& operator=(const LinearAllocator&) = delete;

    void create(size_t capacity, MemoryUsage usage);
    void destroy();

    void* allocate(size_t size);
    void free();

    inline size_t capacity() const { return mCapacity; }
    inline size_t size() const { return mSize; }
    inline size_t remain() const { return mCapacity - mSize; }

private:
    size_t mCapacity;
    size_t mSize;
    char* mBase;
};

} // namespace LD
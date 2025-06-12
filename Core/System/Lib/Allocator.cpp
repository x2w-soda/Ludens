#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Types.h>
#include <Ludens/System/Allocator.h>

namespace LD {

struct LinearAllocatorObj
{
    MemoryUsage usage;
    size_t capacity;
    size_t size;
    byte* base;
};

LinearAllocator LinearAllocator::create(const LinearAllocatorInfo& info)
{
    LinearAllocatorObj* obj = (LinearAllocatorObj*)heap_malloc(sizeof(LinearAllocatorObj), info.usage);
    obj->usage = info.usage;
    obj->capacity = info.capacity;
    obj->size = 0;
    obj->base = nullptr;

    return {obj};
}

void LinearAllocator::destroy(LinearAllocator allocator)
{
    LinearAllocatorObj* obj = allocator;

    if (obj->base)
        heap_free(obj->base);

    heap_free(obj);
}

size_t LinearAllocator::capacity() const
{
    return mObj->capacity;
}

size_t LinearAllocator::size() const
{
    return mObj->size;
}

size_t LinearAllocator::remain() const
{
    return mObj->capacity - mObj->size;
}

void* LinearAllocator::allocate(size_t size)
{
    LD_ASSERT(size <= remain());

    // defer until first allocate() call
    if (mObj->base == nullptr)
        mObj->base = (byte*)heap_malloc(mObj->capacity, mObj->usage);

    void* base = mObj->base + mObj->size;
    mObj->size += size;

    return base;
}

void LinearAllocator::free()
{
    mObj->size = 0;
}

} // namespace LD
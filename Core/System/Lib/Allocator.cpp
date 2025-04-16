#include <Ludens/Header/Assert.h>
#include <Ludens/System/Allocator.h>

namespace LD {

LinearAllocator::LinearAllocator()
    : mCapacity(0), mSize(0), mBase(nullptr)
{
}

LinearAllocator::~LinearAllocator()
{
    if (mBase)
        destroy();
}

void LinearAllocator::create(size_t capacity, MemoryUsage usage)
{
    LD_ASSERT(mBase == nullptr);

    mCapacity = capacity;
    mSize = 0;
    mBase = (char*)heap_malloc(mCapacity, usage);
}

void LinearAllocator::destroy()
{
    LD_ASSERT(mBase != nullptr);

    heap_free(mBase);
    mCapacity = 0;
    mSize = 0;
    mBase = nullptr;
}

void* LinearAllocator::allocate(size_t size)
{
    LD_ASSERT(size <= remain());

    void* base = mBase + mSize;
    mSize += size;

    return base;
}

void LinearAllocator::free()
{
    mSize = 0;
}

} // namespace LD
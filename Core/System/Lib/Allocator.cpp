#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Types.h>
#include <Ludens/System/Allocator.h>

namespace LD {

struct LinearAllocatorObj
{
    size_t capacity;
    size_t size;
    MemoryUsage usage;
    byte* base;
};

LinearAllocator LinearAllocator::create(const LinearAllocatorInfo& info)
{
    LinearAllocatorObj* obj = (LinearAllocatorObj*)heap_malloc(sizeof(LinearAllocatorObj), info.usage);
    obj->usage = info.usage;
    obj->capacity = info.capacity;
    obj->size = 0;
    obj->base = nullptr; // defer until first allocation

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

struct PoolAllocatorObj
{
    struct Block;
    struct Page;

    struct Block
    {
        Block* next;
        Page* owner;
    };

    struct Page
    {
        Page* next;
        Block* freeBlocks;
    };

    size_t blockSize;
    size_t pageSize;
    Page* pageList;
    MemoryUsage usage;
    bool isMultiPage;

    void allocate_page()
    {
        Page* page = (Page*)heap_malloc(sizeof(Page) + blockSize * pageSize, usage);
        page->next = pageList;
        pageList = page;
        page->freeBlocks = (Block*)(page + 1);
        Block* block = page->freeBlocks;

        for (size_t i = 0; i < pageSize - 1; i++)
        {
            block->next = (Block*)((byte*)block + blockSize);
            block->owner = page;
            block = block->next;
        }

        block->owner = page;
        block->next = nullptr;
    }
};

PoolAllocator PoolAllocator::create(const PoolAllocatorInfo& info)
{
    LD_ASSERT(info.blockSize != 0 && info.pageSize > 0);

    PoolAllocatorObj* obj = (PoolAllocatorObj*)heap_malloc(sizeof(PoolAllocatorObj), info.usage);
    obj->usage = info.usage;
    obj->blockSize = info.blockSize + sizeof(PoolAllocatorObj::Block); // each block includes 16-byte header overhead
    obj->pageSize = info.pageSize;
    obj->isMultiPage = info.isMultiPage;
    obj->pageList = nullptr; // defer until first allocation

    return {obj};
}

void PoolAllocator::destroy(PoolAllocator allocator)
{
    PoolAllocatorObj* obj = allocator;

    while (obj->pageList)
    {
        PoolAllocatorObj::Page* page = obj->pageList;
        obj->pageList = obj->pageList->next;
        heap_free(page);
    }

    heap_free(obj);
}

void* PoolAllocator::allocate()
{
    if (!mObj->pageList)
        mObj->allocate_page();

    for (PoolAllocatorObj::Page* page = mObj->pageList; page; page = page->next)
    {
        if (page->freeBlocks)
        {
            PoolAllocatorObj::Block* blk = page->freeBlocks;
            page->freeBlocks = page->freeBlocks->next;
            return blk + 1;
        }
    }

    if (mObj->isMultiPage)
    {
        mObj->allocate_page();
        PoolAllocatorObj::Page* page = mObj->pageList;
        LD_ASSERT(page && page->freeBlocks);

        PoolAllocatorObj::Block* blk = page->freeBlocks;
        page->freeBlocks = page->freeBlocks->next;
        return blk + 1;
    }

    // out of blocks in single page mode
    return nullptr;
}

void PoolAllocator::free(void* block)
{
    PoolAllocatorObj::Block* blk = (PoolAllocatorObj::Block*)block - 1;
    PoolAllocatorObj::Page* page = blk->owner;

    // return block to owning page
    blk->next = page->freeBlocks;
    page->freeBlocks = blk;
}

size_t PoolAllocator::page_count() const
{
    size_t count = 0;

    for (PoolAllocatorObj::Page* page = mObj->pageList; page; page = page->next)
        count++;

    return count;
}

} // namespace LD
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Types.h>
#include <Ludens/System/Allocator.h>

namespace LD {

struct LinearAllocatorObj
{
    struct Page
    {
        Page* next;
        size_t used;
    };

    size_t capacity;   /// byte capacity per page
    Page* pageList;    /// memory pages
    MemoryUsage usage; /// usage domain
    bool isMultiPage;  /// whether allocator paginates

    void allocate_page()
    {
        Page* page = (Page*)heap_malloc(sizeof(Page) + capacity, usage);
        page->next = pageList;
        page->used = 0;
        pageList = page;
    }

    void free()
    {
        Page* page = pageList;

        while (page)
        {
            Page* nextPage = page->next;
            heap_free(page);
            page = nextPage;
        }

        pageList = nullptr;
    }
};

LinearAllocator LinearAllocator::create(const LinearAllocatorInfo& info)
{
    LinearAllocatorObj* obj = (LinearAllocatorObj*)heap_malloc(sizeof(LinearAllocatorObj), info.usage);
    obj->usage = info.usage;
    obj->capacity = info.capacity;
    obj->pageList = nullptr; // defer until first allocation
    obj->isMultiPage = info.isMultiPage;

    return {obj};
}

void LinearAllocator::destroy(LinearAllocator allocator)
{
    LinearAllocatorObj* obj = allocator;

    obj->free();

    heap_free(obj);
}

size_t LinearAllocator::page_count() const
{
    size_t count = 0;

    for (LinearAllocatorObj::Page* page = mObj->pageList; page; page = page->next)
        count++;

    return count;
}

size_t LinearAllocator::capacity() const
{
    return mObj->capacity;
}

size_t LinearAllocator::size() const
{
    size_t size = 0;

    for (LinearAllocatorObj::Page* page = mObj->pageList; page; page = page->next)
    {
        size += page->used;
    }

    return size;
}

size_t LinearAllocator::remain() const
{
    LinearAllocatorObj::Page* currentPage = mObj->pageList;

    if (!currentPage || (mObj->isMultiPage && mObj->capacity == currentPage->used))
        return mObj->capacity; // next page is available at max capacity

    return mObj->capacity - currentPage->used;
}

void* LinearAllocator::allocate(size_t size)
{
    if (size > mObj->capacity)
        return nullptr; // can't satisfy request even in multi-page mode

    if (!mObj->pageList || (mObj->isMultiPage && remain() < size))
        mObj->allocate_page();

    LinearAllocatorObj::Page* currentPage = mObj->pageList;
    LD_ASSERT(currentPage);

    if (currentPage->used + size <= mObj->capacity)
    {
        byte* base = (byte*)currentPage + sizeof(LinearAllocatorObj::Page) + currentPage->used;
        currentPage->used += size;
        return base;
    }

    return nullptr;
}

void LinearAllocator::free()
{
    mObj->free();
}

struct PoolAllocatorObj
{
    struct Block;
    struct Page;

    struct Block
    {
        Block* next; /// next block free for allocation
        Page* owner; /// null if the block is free, otherwise the memory page this block belongs to

        inline bool is_allocated() const
        {
            return owner != nullptr;
        }
    };

    struct Page
    {
        PoolAllocatorObj* obj; /// pool allocator object
        Page* next;            /// linked list of memory pages
        Block* freeBlocks;     /// linked list of blocks free for allocation
        size_t freeBlockCount; /// length of freeBlocks linked list

        Block* get_first_allocated_block()
        {
            if (freeBlockCount == obj->pageSize)
                return nullptr;

            Block* block = (Block*)(this + 1);

            for (size_t i = 0; i < obj->pageSize; i++)
            {
                if (block->is_allocated())
                    return block;

                block = (PoolAllocatorObj::Block*)((byte*)block + obj->blockSize);
            }
        }

        inline size_t allocated_block_count()
        {
            return obj->pageSize - freeBlockCount;
        }
    };

    size_t blockSize;
    size_t pageSize;
    Page* pageList;
    MemoryUsage usage;
    bool isMultiPage;

    void allocate_page()
    {
        Page* page = (Page*)heap_malloc(sizeof(Page) + blockSize * pageSize, usage);
        page->obj = this;
        page->next = pageList;
        pageList = page;
        page->freeBlocks = (Block*)(page + 1);
        page->freeBlockCount = pageSize;
        Block* block = page->freeBlocks;

        for (size_t i = 0; i < pageSize - 1; i++)
        {
            block->next = (Block*)((byte*)block + blockSize);
            block->owner = nullptr;
            block = block->next;
        }

        block->owner = nullptr;
        block->next = nullptr;
    }
};

static_assert(sizeof(PoolAllocatorObj::Block) == 16); // update Allocator.h

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
            LD_ASSERT(page->freeBlockCount > 0);
            PoolAllocatorObj::Block* blk = page->freeBlocks;
            page->freeBlocks = page->freeBlocks->next;
            page->freeBlockCount--;
            blk->owner = page;
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
        page->freeBlockCount--;
        blk->owner = page;
        return blk + 1;
    }

    // out of blocks in single page mode
    return nullptr;
}

void PoolAllocator::free(void* block)
{
    PoolAllocatorObj::Block* blk = (PoolAllocatorObj::Block*)block - 1;
    PoolAllocatorObj::Page* page = blk->owner;

    LD_ASSERT(page);

    // return block to owning page
    blk->next = page->freeBlocks;
    page->freeBlocks = blk;
    page->freeBlockCount++;
}

size_t PoolAllocator::page_count() const
{
    size_t count = 0;

    for (PoolAllocatorObj::Page* page = mObj->pageList; page; page = page->next)
        count++;

    return count;
}

PoolAllocator::Iterator& PoolAllocator::Iterator::operator++()
{
    // jump to next page or return end
    if (mBlocksLeft == 0)
    {
        for (auto* page = ((PoolAllocatorObj::Page*)mPage)->next; page; page = page->next)
        {
            PoolAllocatorObj::Block* block = page->get_first_allocated_block();

            if (block)
            {
                mPage = (byte*)page;
                mBlock = (byte*)block;
                mBlocksLeft = page->allocated_block_count() - 1;
                return *this;
            }
        }

        // complete iteration
        mPage = nullptr;
        mBlock = nullptr;
        return *this;
    }

    auto* obj = (PoolAllocatorObj*)((PoolAllocatorObj::Page*)mPage)->obj;
    auto* block = (PoolAllocatorObj::Block*)mBlock;

    do
    {
        block = (PoolAllocatorObj::Block*)((byte*)block + obj->blockSize);
    } while (!block->is_allocated());

    mBlocksLeft--;
    mBlock = (byte*)block;
    return *this;
}

PoolAllocator::Iterator::Iterator(byte* page, byte* block, size_t blocksLeft)
    : mPage(page), mBlock(block), mBlocksLeft(blocksLeft)
{
}

PoolAllocator::Iterator PoolAllocator::begin()
{
    for (PoolAllocatorObj::Page* page = mObj->pageList; page; page = page->next)
    {
        PoolAllocatorObj::Block* block = page->get_first_allocated_block();

        if (block)
        {
            LD_ASSERT(mObj->pageSize > page->freeBlockCount);
            return Iterator((byte*)page, (byte*)block, page->allocated_block_count() - 1);
        }
    }

    return Iterator(nullptr, nullptr, 0);
}

} // namespace LD
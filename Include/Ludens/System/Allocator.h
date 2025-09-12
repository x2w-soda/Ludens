#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Types.h>
#include <Ludens/System/Memory.h>

namespace LD {

struct LinearAllocatorInfo
{
    MemoryUsage usage; /// the usage space of all allocations made by the allocator
    size_t capacity;   /// page capacity in bytes
    bool isMultiPage;  /// if true, allocate() will create new pages as necessary, otherwise only a single page is allocated.
};

struct LinearAllocator : Handle<struct LinearAllocatorObj>
{
    /// @brief create a linear allocator
    /// @param info linear allocator configuration info
    /// @return allocator handle
    static LinearAllocator create(const LinearAllocatorInfo& info);

    /// @brief destroy the linear allocator, previous calls to allocate() are all freed.
    static void destroy(LinearAllocator allocator);

    /// @brief allocate bytes
    /// @param size number of bytes requested
    void* allocate(size_t size);

    /// @brief frees all previous allocate() calls in one go.
    void free();

    /// @brief number of pages allocated
    size_t page_count() const;

    /// @brief maximum capacity in bytes
    size_t capacity() const;

    /// @brief currently allocated byte size
    size_t size() const;

    /// @brief available bytes for allocate() in the current page
    size_t remain() const;
};

struct PoolAllocatorInfo
{
    MemoryUsage usage; /// the usage space of all allocations made by the allocator
    size_t blockSize;  /// the size of a block in bytes
    size_t pageSize;   /// the number of blocks in a single page
    bool isMultiPage;  /// if true, allocate() will create new pages as necessary, otherwise only a single page is allocated.
};

/// @brief Allocates a pool of fixed-sized blocks.
///        Each page of memory has a fixed number of blocks.
struct PoolAllocator : Handle<struct PoolAllocatorObj>
{
    /// @brief create a pool allocator
    /// @param info pool allocator configuration info
    /// @return allocator handle
    static PoolAllocator create(const PoolAllocatorInfo& info);

    /// @brief destroy the pool allocator, all block allocations by allocate() will be freed.
    static void destroy(PoolAllocator allocator);

    /// @brief allocate a block
    /// @return a new block of memory
    void* allocate();

    /// @brief free a block
    /// @param block a block returned from allocate()
    void free(void* block);

    /// @brief number of pages allocated
    size_t page_count() const;

    /// @brief Iterator to traverse all allocated blocks linearly.
    /// @warning Do not allocate or free blocks when iterating through the pool.
    class Iterator
    {
    public:
        /// @brief prefix increment the iterator
        Iterator& operator++();

        inline bool operator==(const Iterator& other) { return mBlock == other.mBlock; }
        inline bool operator!=(const Iterator& other) { return mBlock != other.mBlock; }

        Iterator(byte* page, byte* block, size_t blocksLeft);

        /// @brief Get the block data pointed by the iterator
        inline void* data() { return mBlock + 16; };

        /// @brief Check if iterator is valid
        inline operator bool() const { return mPage != nullptr; }

    private:
        byte* mPage;
        byte* mBlock;
        size_t mBlocksLeft;
    };

    /// Get iterator to the first allocated block across all pages
    Iterator begin();
};

} // namespace LD
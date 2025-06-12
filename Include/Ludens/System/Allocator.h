#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/System/Memory.h>

namespace LD {

struct LinearAllocatorInfo
{
    MemoryUsage usage; /// the usage space of all allocations made by the allocator
    size_t capacity;   /// capacity in bytes
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

    /// @brief maximum capacity in bytes
    size_t capacity() const;

    /// @brief currently allocated byte size
    size_t size() const;

    /// @brief available bytes for allocate()
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
};

} // namespace LD
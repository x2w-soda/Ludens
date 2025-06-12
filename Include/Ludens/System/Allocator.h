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

} // namespace LD
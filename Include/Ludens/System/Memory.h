#pragma once

#include <cstdlib>
#include <cstdint>
#include <utility>

namespace LD {

enum MemoryUsage : uint32_t
{
    MEMORY_USAGE_MISC = 0,
    MEMORY_USAGE_MEDIA,
    MEMORY_USAGE_RENDER,
    MEMORY_USAGE_UI,
};

struct MemoryProfile
{
    std::size_t peak;
    std::size_t current;
};

/// @brief heap allocation
/// @param size number of bytes
/// @param usage intended usage
void* heap_malloc(std::size_t size, MemoryUsage usage);

/// @brief free a heap allocation
void heap_free(void* ptr);

/// @brief examine memory profile for a given usage 
const MemoryProfile& get_memory_profile(MemoryUsage usage);

template <typename T, typename... TArgs>
T* heap_new(TArgs&&... args, MemoryUsage usage)
{
    T* ptr = (T*)heap_malloc(sizeof(T), usage);
    new (ptr) T(std::forward<TArgs>(args)...);
    return ptr;
}

template <typename T>
void heap_delete(T* ptr)
{
    ptr->~T();
    heap_free(ptr);
}

} // namespace LD

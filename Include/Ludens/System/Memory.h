#pragma once

#include <cstdlib>
#include <cstdint>
#include <utility>

namespace LD {

enum MemoryUsage : uint32_t
{
    MEMORY_USAGE_MISC = 0,
    MEMORY_USAGE_MEDIA,
    MEMORY_USAGE_SERIAL,
    MEMORY_USAGE_RENDER,
    MEMORY_USAGE_UI,
    MEMORY_USAGE_LUA,
    MEMORY_USAGE_JOB_SYSTEM,
    MEMORY_USAGE_TEXT_EDIT,
    MEMORY_USAGE_AUDIO,
    MEMORY_USAGE_PHYSICS,
    MEMORY_USAGE_ASSET,
    MEMORY_USAGE_SCENE,
    MEMORY_USAGE_ENUM_LAST,
};

struct MemoryProfile
{
    MemoryUsage usage;
    std::size_t peak;
    std::size_t current;
};

/// @brief heap allocation
/// @param size number of bytes
/// @param usage intended usage
void* heap_malloc(std::size_t size, MemoryUsage usage);

/// @brief free a heap allocation
void heap_free(void* ptr);

/// @brief duplicate a c string
/// @param cstr a null terminated c string to copy from
/// @param usage intended usage
/// @return a copy of input cstr, must be freed with heap_free
char* heap_strdup(const char* cstr, MemoryUsage usage);

/// @brief examine memory profile for a given usage 
const MemoryProfile& get_memory_profile(MemoryUsage usage);

/// @brief Examine memory leaks for all usages
/// @param leaks If not null, outputs all memory profiles with leaking memory.
/// @return Number of memory profiles that still have allocations not freed.
int get_memory_leaks(MemoryProfile* leaks);

/// @brief get static C string for memory usage
const char* get_memory_usage_cstr(MemoryUsage usage);

template <typename T, typename... TArgs>
T* heap_new(MemoryUsage usage, TArgs&&... args)
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

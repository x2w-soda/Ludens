#include <Ludens/Header/Assert.h>
#include <Ludens/System/Memory.h>
#include <algorithm>
#include <cstring>
#include <mutex>
#include <new>

namespace LD {

struct MemoryHeader
{
    std::size_t size;
    MemoryUsage usage;
};

static MemoryProfile sProfile[MEMORY_USAGE_ENUM_LAST];
static std::mutex sProfileMutex[MEMORY_USAGE_ENUM_LAST];

void* heap_malloc(std::size_t size, MemoryUsage usage)
{
    MemoryHeader* header = (MemoryHeader*)std::malloc(sizeof(MemoryHeader) + size);
    LD_ASSERT(header);

    header->size = size;
    header->usage = usage;

    {
        std::unique_lock<std::mutex> lock(sProfileMutex[usage]);

        sProfile[usage].current += size;
        sProfile[usage].peak = std::max(sProfile[usage].peak, sProfile[usage].current);
    }

    return (void*)(header + 1);
}

void heap_free(void* ptr)
{
    MemoryHeader* header = (MemoryHeader*)ptr - 1;

    {
        std::unique_lock<std::mutex> lock(sProfileMutex[header->usage]);

        sProfile[header->usage].current -= header->size;
    }

    std::free((void*)header);
}

char* heap_strdup(const char* cstr, MemoryUsage usage)
{
    size_t len = strlen(cstr);
    char* str = (char*)heap_malloc(len + 1, usage);
    memcpy(str, cstr, len);
    str[len] = '\0';

    return str;
}

const MemoryProfile& get_memory_profile(MemoryUsage usage)
{
    return sProfile[usage];
}

} // namespace LD

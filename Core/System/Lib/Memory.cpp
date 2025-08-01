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

// clang-format off
struct
{
    MemoryProfile profile;
    const char* cstr;
    std::mutex mutex;
} sTable[]{
    { { MEMORY_USAGE_MISC,},       "MEMORY_USAGE_MISC", },
    { { MEMORY_USAGE_MEDIA,},      "MEMORY_USAGE_MEDIA", },
    { { MEMORY_USAGE_SERIAL,},     "MEMORY_USAGE_SERIAL", },
    { { MEMORY_USAGE_RENDER,},     "MEMORY_USAGE_RENDER", },
    { { MEMORY_USAGE_UI,},         "MEMORY_USAGE_UI", },
    { { MEMORY_USAGE_LUA,},        "MEMORY_USAGE_LUA", },
    { { MEMORY_USAGE_JOB_SYSTEM,}, "MEMORY_USAGE_JOB_SYSTEM", },
    { { MEMORY_USAGE_TEXT_EDIT,},  "MEMORY_USAGE_TEXT_EDIT", },
    { { MEMORY_USAGE_AUDIO,},      "MEMORY_USAGE_AUDIO", },
    { { MEMORY_USAGE_PHYSICS,},    "MEMORY_USAGE_PHYSICS", },
    { { MEMORY_USAGE_ASSET,},      "MEMORY_USAGE_ASSET", },
    { { MEMORY_USAGE_SCENE,},      "MEMORY_USAGE_SCENE", },
};
// clang-format on

static_assert(sizeof(sTable) / sizeof(*sTable) == MEMORY_USAGE_ENUM_LAST);

void* heap_malloc(std::size_t size, MemoryUsage usage)
{
    MemoryHeader* header = (MemoryHeader*)std::malloc(sizeof(MemoryHeader) + size);
    LD_ASSERT(header);

    header->size = size;
    header->usage = usage;

    {
        std::unique_lock<std::mutex> lock(sTable[usage].mutex);

        sTable[usage].profile.current += size;
        sTable[usage].profile.peak = std::max(sTable[usage].profile.peak, sTable[usage].profile.current);
    }

    return (void*)(header + 1);
}

void heap_free(void* ptr)
{
    MemoryHeader* header = (MemoryHeader*)ptr - 1;

    {
        std::unique_lock<std::mutex> lock(sTable[header->usage].mutex);

        sTable[header->usage].profile.current -= header->size;
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
    return sTable[(int)usage].profile;
}

int get_memory_leaks(MemoryProfile* leaks)
{
    int count = 0;

    for (int i = 0; i < MEMORY_USAGE_ENUM_LAST; i++)
    {
        if (sTable[i].profile.current == 0)
            continue;

        if (leaks)
            leaks[count] = sTable[i].profile;

        count++;
    }

    return count;
}

const char* get_memory_usage_cstr(MemoryUsage usage)
{
    return sTable[(int)usage].cstr;
}

} // namespace LD

#include <Ludens/Header/Assert.h>
#include <Ludens/System/Memory.h>
#include <new>

namespace LD {

struct MemoryHeader
{
    std::size_t size;
    MemoryUsage usage;
};

static MemoryProfile sProfile[MEMORY_USAGE_ENUM_LAST];

void* heap_malloc(std::size_t size, MemoryUsage usage)
{
    MemoryHeader* header = (MemoryHeader*)std::malloc(sizeof(MemoryHeader) + size);
    LD_ASSERT(header);

    header->size = size;
    header->usage = usage;

    sProfile[usage].current += size;
    sProfile[usage].peak = std::max(sProfile[usage].peak, sProfile[usage].current);

    return (void*)(header + 1);
}

void heap_free(void* ptr)
{
    MemoryHeader* header = (MemoryHeader*)ptr - 1;

    sProfile[header->usage].current -= header->size;

    std::free((void*)header);
}

const MemoryProfile& get_memory_profile(MemoryUsage usage)
{
    return sProfile[usage];
}

} // namespace LD
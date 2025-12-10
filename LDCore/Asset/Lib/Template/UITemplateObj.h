#pragma once

#include <Ludens/System/Allocator.h>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace LD {

struct UITemplateEntry;

/// @brief UI template implementation.
struct UITemplateObj
{
    PoolAllocator entryPA;
    LinearAllocator LA;
    std::vector<UITemplateEntry*> entries;
    std::unordered_map<uint32_t, std::vector<uint32_t>> hierarchy;

    void reset()
    {
        for (auto ite = entryPA.begin(); ite; ++ite)
            entryPA.free(ite.data());

        entries.clear();
        hierarchy.clear();
        LA.free();
    }
};

} // namespace LD
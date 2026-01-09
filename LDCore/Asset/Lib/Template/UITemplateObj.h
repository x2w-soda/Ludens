#pragma once

#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Memory/Allocator.h>

#include <cstdint>
#include <unordered_map>

namespace LD {

struct UITemplateEntry;

/// @brief UI template implementation.
struct UITemplateObj
{
    PoolAllocator entryPA;
    LinearAllocator LA;
    Vector<UITemplateEntry*> entries;
    HashMap<uint32_t, Vector<uint32_t>> hierarchy;

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
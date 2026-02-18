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

    UITemplateEntry* allocate_entry()
    {
        UITemplateEntry* entry = (UITemplateEntry*)entryPA.allocate();
        new (entry) UITemplateEntry();

        return entry;
    }

    void reset()
    {
        for (auto it = entryPA.begin(); it; ++it)
        {
            UITemplateEntry* entry = (UITemplateEntry*)it.data();
            entry->~UITemplateEntry();
            entryPA.free(entry);
        }

        entries.clear();
        hierarchy.clear();
        LA.free();
    }
};

} // namespace LD
#pragma once

#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Memory/Allocator.h>

#include <cstdint>

namespace LD {

struct UITemplateEntry;

/// @brief UI template implementation.
struct UITemplateObj
{
    PoolAllocator entryPA;
    LinearAllocator LA;
    Vector<UITemplateEntry*> entries;
    HashMap<uint32_t, Vector<uint32_t>> hierarchy;

    UITemplateEntry* allocate_entry(UIWidgetType type);

    void reset();
};

} // namespace LD
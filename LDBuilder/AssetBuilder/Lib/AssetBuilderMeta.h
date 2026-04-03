#pragma once

#include <LudensBuilder/AssetBuilder/AssetImporter.h>
#include <LudensBuilder/AssetBuilder/AssetType/AssetBuilders.h>

namespace LD {

/// @brief Asset builder polymorphism by AssetType
struct AssetBuilderMeta
{
    AssetImportInfo* (*alloc_info)();
    void (*free_info)(AssetImportInfo*);
    void (*import_fn)(void*);
};

extern AssetBuilderMeta sBuilderMeta[];

} // namespace LD
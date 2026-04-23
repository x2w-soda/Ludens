#pragma once

#include <LudensBuilder/AssetBuilder/AssetBuilderDef.h>
#include <LudensBuilder/AssetBuilder/AssetImporter.h>
#include <LudensBuilder/AssetBuilder/AssetSource/AssetSources.h>

namespace LD {

/// @brief Asset polymorphic creation by AssetBuildType
struct AssetCreateMeta
{
    AssetCreateInfo* (*allocInfo)();
    void (*freeInfo)(AssetCreateInfo*);
    bool (*createFn)(AssetCreateInfo* createInfo, std::string& err);
    bool (*prepareImportFn)(const AssetCreateInfo* createInfo, AssetImportInfo* importInfo, std::string& err);
};

/// @brief Asset polymorphic import by AssetType
struct AssetImportMeta
{
    AssetImportInfo* (*allocInfo)();
    void (*freeInfo)(AssetImportInfo*);
    void (*importFn)(void*);
    bool (*setSrcFiles)(AssetImportInfo*, size_t, const FS::Path*);
};

extern AssetCreateMeta sCreateMeta[];
extern AssetImportMeta sImportMeta[];

} // namespace LD
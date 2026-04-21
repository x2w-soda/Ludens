#pragma once

#include <Ludens/Header/Handle.h>
#include <LudensBuilder/AssetBuilder/AssetBuilderDef.h>

namespace LD {

/// @brief Generates Assets to be imported.
struct AssetBuilder : Handle<struct AssetBuilderObj>
{
    static AssetBuilder create();
    static void destroy(AssetBuilder builder);

    AssetCreateInfo* allocate_create_info(AssetCreateType type);
    void free_create_info(AssetCreateInfo* info);

    /// @brief Try create asset data from create info.
    bool create_asset(AssetCreateInfo* info, std::string& err);

    /// @brief Try fill out import info from create info.
    bool prepare_import(const AssetCreateInfo* srcInfo, AssetImportInfo* dstInfo, std::string& err);
};

} // namespace LD
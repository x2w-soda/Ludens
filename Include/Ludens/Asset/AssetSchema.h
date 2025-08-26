#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Media/Format/TOML.h>

namespace LD {

/// @brief Schema for listing all Assets under the current framework version.
struct AssetSchema
{
    static void load_assets(AssetManager manager, TOMLDocument doc);
};

} // namespace LD
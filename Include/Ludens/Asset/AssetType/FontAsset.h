#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Media/Font.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

/// @brief Font asset handle.
struct FontAsset : Asset
{
    /// @brief Get font handle.
    Font get_font();

    /// @brief Get font atlas handle.
    FontAtlas get_font_atlas();
};

} // namespace LD
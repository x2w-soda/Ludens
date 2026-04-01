#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/System/FileSystem.h>
#include <cstdint>

namespace LD {

/// @brief TextureCube asset handle.
struct TextureCubeAsset : Asset
{
    /// @brief Get bitmap for cubemap.
    Bitmap get_bitmap() const;
};



} // namespace LD
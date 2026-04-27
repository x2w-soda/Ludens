#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

enum TextureCompression
{
    /// @brief LZ4 compressed bitmap
    TEXTURE_COMPRESSION_LZ4 = 0,
};

extern struct PropertyMetaTable gTexture2DPropMetaTable;

/// @brief Texture2D asset handle.
struct Texture2DAsset : Asset
{
    /// @brief Unload asset from RAM.
    void unload();

    /// @brief Get bitmap in RAM.
    Bitmap get_bitmap();

    /// @brief Get desired texture sampling mode.
    RSamplerInfo get_sampler_hint() const;
};

} // namespace LD
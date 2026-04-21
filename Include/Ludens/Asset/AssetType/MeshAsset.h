#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Media/Model.h>
#include <filesystem>

namespace LD {

/// @brief Mesh asset handle.
struct MeshAsset : Asset
{
    /// @brief get mesh binary data
    class ModelBinary* data();

    /// @brief Unload asset from RAM. The handle becomes null afterwards.
    void unload();
};

} // namespace LD
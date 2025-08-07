#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Header/Handle.h>
#include <filesystem>

namespace LD {

/// @brief Ludens framework asset utilities.
struct AssetUtil : Handle<struct AssetUtilObj>
{
    static AssetUtil create();
    static void destroy(AssetUtil util);

    /// @brief import a Texture2D asset from source file
    /// @return true on success
    bool import_texture_2d(const std::filesystem::path& sourcePath);

    /// @brief import a Mesh asset from source file
    /// @return true on success
    bool import_mesh(const std::filesystem::path& sourcePath);
};

} // namespace LD
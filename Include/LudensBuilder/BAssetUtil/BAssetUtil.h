#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/System/FileSystem.h>
#include <filesystem>

namespace LD {

/// @brief Ludens framework asset utilities.
struct AssetUtil : Handle<struct AssetUtilObj>
{
    static AssetUtil create();
    static void destroy(AssetUtil util);

    /// @brief Import a Blob asset from source file.
    /// @return True on success.
    bool import_blob(const FS::Path& sourcePath);

    /// @brief Import a Texture2D asset from source file.
    /// @return True on success.
    bool import_texture_2d(const FS::Path& sourcePath);

    /// @brief Import a TextureCube asset from source PNG faces.
    /// @param sourcePath Path to a directory containing 6 cubemap PNGs. The faces are expected to be
    ///        "px.png", "nx.png", "py.png", "ny.png", "pz.png" and "nz.png".
    /// @return True on success.
    bool import_texture_cube(const FS::Path& sourcePath);

    /// @brief Import a Mesh asset from source file.
    /// @return True on success.
    bool import_font(const FS::Path& sourcePath);

    /// @brief Import a Mesh asset from source file.
    /// @return True on success.
    bool import_mesh(const FS::Path& sourcePath);

    /// @brief Import an AudioClip asset from source file.
    /// @return True on success.
    bool import_audio_clip(const FS::Path& sourcePath);
};

} // namespace LD
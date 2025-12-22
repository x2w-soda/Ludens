#pragma once

#include <Ludens/Header/Handle.h>
#include <filesystem>

namespace LD {

struct RenderUtil : Handle<struct RenderUtilObj>
{
    static RenderUtil create();
    static void destroy(RenderUtil util);

    /// @brief samples an equirectangular environment map and saves 6 cubemap faces on disk
    /// @param path path to an equirectangular texture, usually with hdr/hdri file extensions.
    /// @param dstDirectory the directory to store 6 cubemap faces
    void from_equirectangular_to_faces(const std::filesystem::path& path, const std::filesystem::path& dstDirectory);
};

} // namespace LD
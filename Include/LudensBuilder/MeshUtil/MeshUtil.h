#pragma once

#include <Ludens/Header/Handle.h>
#include <filesystem>

namespace LD {

/// @brief 3D Model mesh utilities.
struct MeshUtil : Handle<struct MeshUtilObj>
{
    static MeshUtil create();
    static void destroy(MeshUtil util);

    /// @brief Extracts struct MeshVertex and uint32_t face indices into a cpp file.
    /// @param path path to 3D model.
    /// @return true on success
    bool extract_mesh_vertex(const std::filesystem::path& path);
};

} // namespace LD
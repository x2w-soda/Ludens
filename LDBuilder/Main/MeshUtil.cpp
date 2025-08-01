#include "MeshUtil.h"
#include <Ludens/Header/Math/Math.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Media/Model.h>
#include <Ludens/System/FileSystem.h>
#include <filesystem>
#include <iomanip>

namespace fs = std::filesystem;

namespace LD {

static Log sLog("LDBuilder");

template <typename T>
static inline T checkz(T v)
{
    return is_zero_epsilon(v) ? (T)0 : v;
}

MeshUtil MeshUtil::create()
{
    // SPACE: define a MeshUtilObj to retain shared state if we need to

    return {};
}

void MeshUtil::destroy(MeshUtil util)
{
}

bool MeshUtil::extract_mesh_vertex(const std::filesystem::path& path)
{
    if (!fs::exists(path))
    {
        sLog.warn("3D model does not exist: {}", path.string());
        return false;
    }

    Model model = Model::load_gltf_model(path.string().c_str());

    uint32_t vertexCount, indexCount;
    MeshVertex* vertices = model.get_vertices(vertexCount);
    uint32_t* indices = model.get_indices(indexCount);

    std::stringstream ss;
    ss << std::setprecision(5);
    ss << "// This .cpp file is an intermediate file generated by LDBuilder.\n";
    ss << "// Containing mesh data extracted from: " << path << '\n';
    ss << "#include <cstdint>\n";
    ss << "struct Vec2 { float x, y; };\n";
    ss << "struct Vec3 { float x, y, z; };\n";
    ss << "struct MeshVertex {\n";
    ss << "    Vec3 pos;\n";
    ss << "    Vec3 normal;\n";
    ss << "    Vec2 uv;\n";
    ss << "};\n";

    ss << "const size_t sVertexCount = " << vertexCount << ";\n";
    ss << "static MeshVertex sVertices[sVertexCount] = {\n";
    for (uint32_t i = 0; i < vertexCount; i++)
    {
        const MeshVertex& v = vertices[i];
        ss << "    {{" << checkz(v.pos.x) << ", " << checkz(v.pos.y) << ", " << checkz(v.pos.z) << "},";
        ss << " {" << checkz(v.normal.x) << ", " << checkz(v.normal.y) << ", " << checkz(v.normal.z) << "},";
        ss << " {" << checkz(v.uv.x) << ", " << checkz(v.uv.y) << "}},\n";
    }
    ss << "};\n";
    ss << "const size_t sIndexCount = " << indexCount << ";\n";
    ss << "static uint32_t sIndices[sIndexCount] = {\n    ";
    for (uint32_t i = 0; i < indexCount; i++)
    {
        ss << indices[i] << ", ";
    }
    ss << "\n};\n";

    std::string cppString = ss.str();
    fs::path cppPath = path;
    cppPath += fs::path(".cpp");
    bool success = FS::write_file(cppPath, cppString.size(), (const byte*)cppString.data());

    if (!success)
    {
        sLog.warn("failed to write extracted data to: {}", cppPath.string());
        return false;
    }

    return true;
}

} // namespace LD
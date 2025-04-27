#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Header/Math/Vec3.h>
#include <Ludens/Header/Math/Vec4.h>
#include <Ludens/Header/Math/Quat.h>
#include <vector>
#include <string>

namespace LD {

/// @brief basic unit for describing mesh geometry
struct MeshVertex
{
    Vec3 pos;
    Vec3 normal;
    Vec2 uv;
};

/// @brief material information, orthogonal to mesh geometry.
struct MeshMaterial
{
    Vec4 baseColorFactor;
    int baseColorTextureIndex;
};

/// @brief describes how a portion of MeshData is rendered with a MeshMaterial
struct MeshPrimitive
{
    uint32_t indexStart;
    uint32_t indexCount;
    uint32_t vertexCount;
    MeshMaterial* material;
};

/// @brief mesh heirachy
struct MeshNode
{
    MeshNode* parent;
    Vec3 translation;
    Vec3 scale;
    Quat rotation;
    std::string name;
    std::vector<MeshPrimitive> primitives;
    std::vector<MeshNode*> children;
};

struct Model : Handle<struct ModelObj>
{
    static Model load_gltf_model(const char* path);
    static void destroy(Model model);

    MeshVertex* get_vertices(int& vertexCount);
    uint32_t* get_indices(int& indexCount);
    MeshNode** get_roots(int& rootCount);
    Bitmap* get_textures(int& textureCount);
    MeshMaterial* get_materials(int& materialCount);
};

} // namespace LD
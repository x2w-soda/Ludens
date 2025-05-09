#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/Header/Math/Quat.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Header/Math/Vec3.h>
#include <Ludens/Header/Math/Vec4.h>
#include <Ludens/Media/Bitmap.h>
#include <string>
#include <vector>

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
    uint32_t vertexStart;
    uint32_t vertexCount;
    MeshMaterial* material;
};

/// @brief mesh heirachy
struct MeshNode
{
    MeshNode* parent;
    Mat4 localTransform;
    std::string name;
    std::vector<MeshPrimitive> primitives;
    std::vector<MeshNode*> children;
};

struct Model : Handle<struct ModelObj>
{
    static Model load_gltf_model(const char* path);
    static void destroy(Model model);

    /// @brief get model vertices
    /// @warning the returned pointer is transient
    MeshVertex* get_vertices(uint32_t& vertexCount);

    /// @brief get model 32-bit indices
    /// @warning the returned pointer is transient
    uint32_t* get_indices(uint32_t& indexCount);

    /// @brief get model root nodes
    /// @warning the returned pointer is transient
    MeshNode** get_roots(uint32_t& rootCount);

    /// @brief get model textures
    /// @warning the returned pointer is transient
    Bitmap* get_textures(uint32_t& textureCount);

    /// @brief get model materials
    /// @warning the returned pointer is transient
    MeshMaterial* get_materials(uint32_t& materialCount);

    /// @brief get the total number of primitives across all nodes
    void get_primitive_count(uint32_t& primitiveCount);

    /// @brief get model local space AABB
    void get_aabb(Vec3& minPos, Vec3& maxPos);

    /// @brief Traverse MeshNode tree and transform each vertex to world space,
    ///        subsequent calls to get_vertices will return world space vertices.
    ///        Each MeshNode transform matrix will be reset to identity matrix.
    void apply_node_transform();
};

} // namespace LD
#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/Header/Math/Quat.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Header/Math/Vec3.h>
#include <Ludens/Header/Math/Vec4.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Serial/Serial.h>
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
    int32_t baseColorTextureIndex;
};

/// @brief describes how a portion of MeshData is rendered with a MeshMaterial
struct MeshPrimitive
{
    uint32_t indexStart;
    uint32_t indexCount;
    uint32_t vertexStart;
    uint32_t vertexCount;
    int32_t matIndex;
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
    /// @param primitiveCount output number of primitves across all nodes
    /// @param primitives if not null, MeshPrimitives are copied over
    void get_primitives(uint32_t& primitiveCount, MeshPrimitive* primitives);

    /// @brief Traverse MeshNode tree and transform each vertex to world space,
    ///        subsequent calls to get_vertices will return world space vertices.
    ///        Each MeshNode transform matrix will be reset to identity matrix.
    void apply_node_transform();
};

class ModelBinary
{
public:
    std::vector<MeshPrimitive> prims;
    std::vector<MeshMaterial> mats;
    std::vector<Bitmap> textures;
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;

    ModelBinary() = default;
    ModelBinary(const ModelBinary&) = delete;
    ~ModelBinary();
    ModelBinary& operator=(const ModelBinary&) = delete;

    /// @brief create binary format of a rigid mesh, this format drops MeshNode
    ///        hierachy information, flattens MeshPrimitives into an array,
    ///        and stores MeshVertex in world space.
    void from_rigid_mesh(Model& model);

    static void serialize(Serializer& serializer, const ModelBinary& bin);
    static void deserialize(Serializer& serializer, ModelBinary& bin);

private:
    bool mIsTextureOwner = false;
};

void get_mesh_vertex_aabb(const MeshVertex* vertices, uint32_t vertexCount, Vec3& min, Vec3& max);

} // namespace LD
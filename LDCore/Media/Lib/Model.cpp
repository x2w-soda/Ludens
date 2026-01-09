#include <Ludens/Header/Math/Mat3.h>
#include <Ludens/Media/Model.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Serial.h>
#include <iostream>

#include "Format/TinygltfLoader.h"
#include "ModelObj.h"

namespace LD {

static void apply_node_transform_recursive(MeshVertex* worldVertices, const MeshVertex* localVertices, MeshNode* root, const Mat4& parentWorldTransform)
{
    if (!root)
        return;

    Mat4 worldTransform = root->localTransform * parentWorldTransform;
    root->localTransform = Mat4(1.0f);
    Mat3 normalMat = Mat3::transpose(Mat3::inverse(worldTransform.as_mat3()));

    for (const MeshPrimitive& prim : root->primitives)
    {
        const MeshVertex* lv = localVertices + prim.vertexStart;
        MeshVertex* wv = worldVertices + prim.vertexStart;

        for (uint32_t i = 0; i < prim.vertexCount; i++)
        {
            wv[i].uv = lv[i].uv;
            wv[i].pos = (worldTransform * Vec4(lv[i].pos, 1.0f)).as_vec3();
            wv[i].normal = Vec3::normalize(normalMat * lv[i].normal);
        }
    }

    for (MeshNode* child : root->children)
    {
        apply_node_transform_recursive(worldVertices, localVertices, child, worldTransform);
    }
}

Model Model::load_gltf_model(const char* path)
{
    LD_PROFILE_SCOPE;

    ModelObj* obj = heap_new<ModelObj>(MEMORY_USAGE_MEDIA);
    obj->hasAppliedNodeTransform = false;

    TinygltfLoader loader;

    bool result = loader.load_from_file(obj, path);
    if (!result)
    {
        heap_free(obj);
        return {};
    }

    return {obj};
}

void Model::destroy(Model model)
{
    LD_PROFILE_SCOPE;

    ModelObj* obj = model;

    for (Bitmap texture : obj->textures)
        Bitmap::destroy(texture);

    for (MeshNode* node : obj->nodes)
        heap_delete(node);

    heap_delete(obj);
}

MeshVertex* Model::get_vertices(uint32_t& vertexCount)
{
    if (mObj->vertices.empty())
    {
        vertexCount = 0;
        return nullptr;
    }

    vertexCount = (uint32_t)mObj->vertices.size();
    return mObj->vertices.data();
}

uint32_t* Model::get_indices(uint32_t& indexCount)
{
    if (mObj->indices.empty())
    {
        indexCount = 0;
        return nullptr;
    }

    indexCount = (uint32_t)mObj->indices.size();
    return mObj->indices.data();
}

MeshNode** Model::get_roots(uint32_t& rootCount)
{
    if (mObj->roots.empty())
    {
        rootCount = 0;
        return nullptr;
    }

    rootCount = (uint32_t)mObj->roots.size();
    return mObj->roots.data();
}

Bitmap* Model::get_textures(uint32_t& textureCount)
{
    if (mObj->textures.empty())
    {
        textureCount = 0;
        return nullptr;
    }

    textureCount = (uint32_t)mObj->textures.size();
    return mObj->textures.data();
}

MeshMaterial* Model::get_materials(uint32_t& materialCount)
{
    if (mObj->materials.empty())
    {
        materialCount = 0;
        return nullptr;
    }

    materialCount = (uint32_t)mObj->materials.size();
    return mObj->materials.data();
}

static void get_primitives_recursive(MeshNode* root, uint32_t& primIndex, MeshPrimitive* prims)
{
    if (!root)
        return;

    if (prims)
    {
        for (size_t i = 0; i < root->primitives.size(); i++)
            prims[primIndex + i] = root->primitives[i];
    }

    primIndex += (uint32_t)root->primitives.size();

    for (MeshNode* child : root->children)
    {
        get_primitives_recursive(child, primIndex, prims);
    }
}

void Model::get_primitives(uint32_t& primitiveCount, MeshPrimitive* prim)
{
    primitiveCount = 0;

    for (MeshNode* root : mObj->roots)
        get_primitives_recursive(root, primitiveCount, prim);
}

void Model::apply_node_transform()
{
    LD_PROFILE_SCOPE;

    if (mObj->hasAppliedNodeTransform)
        return;

    mObj->hasAppliedNodeTransform = true;

    // The vertex range of different mesh primitives may overlap, so we can't
    // apply the transform in-place.
    std::vector<MeshVertex> worldVertices = mObj->vertices;

    for (MeshNode* root : mObj->roots)
    {
        apply_node_transform_recursive(worldVertices.data(), mObj->vertices.data(), root, Mat4(1.0f));
    }

    // safe to replace local space vertices with world space ones
    mObj->vertices = std::move(worldVertices);
}

ModelBinary::~ModelBinary()
{
    if (mIsTextureOwner)
    {
        for (Bitmap texture : textures)
            Bitmap::destroy(texture);

        textures.clear();
    }
}

void ModelBinary::from_rigid_mesh(Model& model)
{
    LD_PROFILE_SCOPE;

    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t textureCount;
    uint32_t primCount;
    uint32_t matCount;

    MeshMaterial* m = model.get_materials(matCount);
    MeshVertex* v = model.get_vertices(vertexCount);
    uint32_t* i = model.get_indices(indexCount);
    Bitmap* t = model.get_textures(textureCount);
    model.get_primitives(primCount, nullptr);

    vertices.resize(vertexCount);
    std::copy(v, v + vertexCount, vertices.data());

    indices.resize(indexCount);
    std::copy(i, i + indexCount, indices.data());

    textures.resize(textureCount);
    std::copy(t, t + textureCount, textures.data());

    mats.resize(matCount);
    std::copy(m, m + matCount, mats.data());

    prims.resize(primCount);
    model.get_primitives(primCount, prims.data());
}

bool ModelBinary::serialize(Serializer& serial, const ModelBinary& bin)
{
    LD_PROFILE_SCOPE;

    serial.write_chunk_begin("SIZE");
    serial.write_u32((uint32_t)bin.vertices.size());
    serial.write_u32((uint32_t)bin.indices.size());
    serial.write_u32((uint32_t)bin.textures.size());
    serial.write_u32((uint32_t)bin.mats.size());
    serial.write_u32((uint32_t)bin.prims.size());
    serial.write_chunk_end();

    serial.write_chunk_begin("VTX.");
    for (const MeshVertex& v : bin.vertices)
    {
        serial.write_vec3(v.pos);
        serial.write_vec3(v.normal);
        serial.write_vec2(v.uv);
    }
    serial.write_chunk_end();

    serial.write_chunk_begin("IDX.");
    for (uint32_t index : bin.indices)
        serial.write_u32(index);
    serial.write_chunk_end();

    serial.write_chunk_begin("TEX.");
    for (Bitmap texture : bin.textures)
    {
        texture.set_compression(BITMAP_COMPRESSION_LZ4);
        Bitmap::serialize(serial, texture);
    }
    serial.write_chunk_end();

    serial.write_chunk_begin("MAT.");
    for (const MeshMaterial& mat : bin.mats)
    {
        serial.write_vec4(mat.baseColorFactor);
        serial.write_f32(mat.metallicFactor);
        serial.write_f32(mat.roughnessFactor);
        serial.write_i32(mat.baseColorTextureIndex);
        serial.write_i32(mat.normalTextureIndex);
        serial.write_i32(mat.metallicRoughnessTextureIndex);
    }
    serial.write_chunk_end();

    serial.write_chunk_begin("PRIM");
    for (const MeshPrimitive& prim : bin.prims)
    {
        serial.write_u32(prim.indexStart);
        serial.write_u32(prim.indexCount);
        serial.write_u32(prim.vertexStart);
        serial.write_u32(prim.vertexCount);
        serial.write_i32(prim.matIndex);
    }
    serial.write_chunk_end();

    return true;
}

bool ModelBinary::deserialize(Deserializer& serial, ModelBinary& bin)
{
    LD_PROFILE_SCOPE;

    bin.mIsTextureOwner = true;

    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t textureCount;
    uint32_t matCount;
    uint32_t primCount;

    uint32_t chunkSize;
    std::string chunkName;
    chunkName.resize(4);

    serial.read_chunk(chunkName.data(), chunkSize);
    if (chunkName != "SIZE")
        return false;

    serial.read_u32(vertexCount);
    serial.read_u32(indexCount);
    serial.read_u32(textureCount);
    serial.read_u32(matCount);
    serial.read_u32(primCount);

    serial.read_chunk(chunkName.data(), chunkSize);
    if (chunkName != "VTX.")
        return false;

    bin.vertices.resize(vertexCount);
    for (uint32_t i = 0; i < vertexCount; i++)
    {
        MeshVertex& v = bin.vertices[i];
        serial.read_vec3(v.pos);
        serial.read_vec3(v.normal);
        serial.read_vec2(v.uv);
    }

    serial.read_chunk(chunkName.data(), chunkSize);
    if (chunkName != "IDX.")
        return false;

    bin.indices.resize(indexCount);
    for (uint32_t i = 0; i < indexCount; i++)
        serial.read_u32(bin.indices[i]);

    serial.read_chunk(chunkName.data(), chunkSize);
    if (chunkName != "TEX.")
        return false;

    bin.textures.resize(textureCount);
    for (uint32_t i = 0; i < textureCount; i++)
        Bitmap::deserialize(serial, bin.textures[i]);

    serial.read_chunk(chunkName.data(), chunkSize);
    if (chunkName != "MAT.")
        return false;

    bin.mats.resize(matCount);
    for (uint32_t i = 0; i < matCount; i++)
    {
        MeshMaterial& mat = bin.mats[i];
        serial.read_vec4(mat.baseColorFactor);
        serial.read_f32(mat.metallicFactor);
        serial.read_f32(mat.roughnessFactor);
        serial.read_i32(mat.baseColorTextureIndex);
        serial.read_i32(mat.normalTextureIndex);
        serial.read_i32(mat.metallicRoughnessTextureIndex);
    }

    serial.read_chunk(chunkName.data(), chunkSize);
    if (chunkName != "PRIM")
        return false;

    bin.prims.resize(primCount);
    for (uint32_t i = 0; i < primCount; i++)
    {
        MeshPrimitive& prim = bin.prims[i];
        serial.read_u32(prim.indexStart);
        serial.read_u32(prim.indexCount);
        serial.read_u32(prim.vertexStart);
        serial.read_u32(prim.vertexCount);
        serial.read_i32(prim.matIndex);
    }

    return true;
}

void get_mesh_vertex_aabb(const MeshVertex* vertices, uint32_t vertexCount, Vec3& min, Vec3& max)
{
    if (vertexCount == 0)
    {
        min = Vec3(0.0f);
        max = Vec3(0.0f);
        return;
    }

    min = max = vertices[0].pos;

    for (uint32_t i = 0; i < vertexCount; i++)
    {
        const MeshVertex& vertex = vertices[i];
        min.x = std::min<float>(min.x, vertex.pos.x);
        min.y = std::min<float>(min.y, vertex.pos.y);
        min.z = std::min<float>(min.z, vertex.pos.z);
        max.x = std::max<float>(max.x, vertex.pos.x);
        max.y = std::max<float>(max.y, vertex.pos.y);
        max.z = std::max<float>(max.z, vertex.pos.z);
    }
}

} // namespace LD
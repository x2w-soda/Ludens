#include "ModelObj.h"
#include "TinygltfLoader.h"
#include <Ludens/Header/Math/Mat3.h>
#include <Ludens/Media/Model.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/Memory.h>
#include <iostream>

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
    obj->hasCalculatedAABB = false;
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

static uint32_t get_primitive_count_recursive(MeshNode* root)
{
    if (!root)
        return 0;

    uint32_t count = (uint32_t)root->primitives.size();
    for (MeshNode* child : root->children)
        count += get_primitive_count_recursive(child);

    return count;
}

void Model::get_primitive_count(uint32_t& primitiveCount)
{
    primitiveCount = 0;

    for (MeshNode* root : mObj->roots)
        primitiveCount += get_primitive_count_recursive(root);
}

void Model::get_aabb(Vec3& minPos, Vec3& maxPos)
{
    LD_PROFILE_SCOPE;

    if (mObj->hasCalculatedAABB)
    {
        minPos = mObj->minPos;
        maxPos = mObj->maxPos;
        return;
    }

    if (mObj->vertices.empty())
    {
        printf("Model::get_aabb model not loaded\n");
        return;
    }

    mObj->hasCalculatedAABB = true;

    minPos = maxPos = mObj->vertices[0].pos;

    for (const MeshVertex& vertex : mObj->vertices)
    {
        minPos.x = std::min<float>(minPos.x, vertex.pos.x);
        minPos.y = std::min<float>(minPos.y, vertex.pos.y);
        minPos.z = std::min<float>(minPos.z, vertex.pos.z);
        maxPos.x = std::max<float>(maxPos.x, vertex.pos.x);
        maxPos.y = std::max<float>(maxPos.y, vertex.pos.y);
        maxPos.z = std::max<float>(maxPos.z, vertex.pos.z);
    }

    mObj->minPos = minPos;
    mObj->maxPos = maxPos;
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

} // namespace LD
#include "ModelObj.h"
#include "TinygltfLoader.h"
#include <Ludens/Media/Model.h>
#include <Ludens/System/Memory.h>
#include <iostream>

namespace LD {

Model Model::load_gltf_model(const char* path)
{
    ModelObj* obj = heap_new<ModelObj>(MEMORY_USAGE_MEDIA);
    obj->hasCalculatedAABB = false;

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
    ModelObj* obj = model;

    for (Bitmap texture : obj->textures)
        Bitmap::destroy(texture);

    for (MeshNode* node : obj->nodes)
        heap_delete(node);

    heap_delete(obj);
}

MeshVertex* Model::get_vertices(int& vertexCount)
{
    if (mObj->vertices.empty())
    {
        vertexCount = 0;
        return nullptr;
    }

    vertexCount = (int)mObj->vertices.size();
    return mObj->vertices.data();
}

uint32_t* Model::get_indices(int& indexCount)
{
    if (mObj->indices.empty())
    {
        indexCount = 0;
        return nullptr;
    }

    indexCount = (int)mObj->indices.size();
    return mObj->indices.data();
}

MeshNode** Model::get_roots(int& rootCount)
{
    if (mObj->roots.empty())
    {
        rootCount = 0;
        return nullptr;
    }

    rootCount = (int)mObj->roots.size();
    return mObj->roots.data();
}

Bitmap* Model::get_textures(int& textureCount)
{
    if (mObj->textures.empty())
    {
        textureCount = 0;
        return nullptr;
    }

    textureCount = (int)mObj->textures.size();
    return mObj->textures.data();
}

MeshMaterial* Model::get_materials(int& materialCount)
{
    if (mObj->materials.empty())
    {
        materialCount = 0;
        return nullptr;
    }

    materialCount = (int)mObj->materials.size();
    return mObj->materials.data();
}

void Model::get_aabb(Vec3& minPos, Vec3& maxPos)
{
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

} // namespace LD
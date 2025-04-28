#include "ModelObj.h"
#include "TinygltfLoader.h"
#include <Ludens/Media/Model.h>
#include <Ludens/System/Memory.h>
#include <iostream>

namespace LD {

Model Model::load_gltf_model(const char* path)
{
    ModelObj* obj = heap_new<ModelObj>(MEMORY_USAGE_MEDIA);
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

} // namespace LD
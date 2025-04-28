#pragma once

#include <Ludens/Header/Math/Vec3.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Media/Model.h>
#include <vector>

namespace LD {

struct ModelObj
{
    std::vector<Bitmap> textures;
    std::vector<MeshMaterial> materials;
    std::vector<MeshNode*> nodes; /// all nodes in this model
    std::vector<MeshNode*> roots; /// the subset of nodes in this model that have no parents
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;
    Vec3 minPos;
    Vec3 maxPos;
    bool hasCalculatedAABB;
};

} // namespace LD
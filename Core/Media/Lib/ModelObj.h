#pragma once

#include <Ludens/Media/Bitmap.h>
#include <Ludens/Media/Model.h>
#include <vector>

namespace LD {

struct ModelObj
{
    std::vector<Bitmap> textures;
    std::vector<MeshMaterial> materials;
    std::vector<MeshNode*> nodes;         /// all nodes in this model
    std::vector<MeshNode*> roots;         /// the subset of nodes in this model that have no parents
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;
};

} // namespace LD
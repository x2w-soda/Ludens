#pragma once

#include <tiny_gltf.h>
#include <cstdint>

namespace LD {

struct ModelObj;
struct MeshNode;

class TinygltfLoader
{
public:
    bool load_from_file(ModelObj* obj, const char* path);

private:
    bool load_model();
    bool load_images();
    bool load_materials();
    bool load_node(tinygltf::Node& tinyNode, uint32_t nodeIndex, MeshNode* parent);
    bool load_mesh(tinygltf::Mesh& tinyMesh, MeshNode* node);
    void scan_node_primitives(tinygltf::Node& tinyNode);

    ModelObj* mObj = nullptr;
    tinygltf::Model mTinyModel;
    tinygltf::TinyGLTF mContext;
    uint32_t mVertexCount = 0;
    uint32_t mVertexBase = 0;
    uint32_t mIndexCount = 0;
    uint32_t mIndexBase = 0;
};

} // namespace LD
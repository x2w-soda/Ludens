#pragma once

#include <Ludens/Media/Bitmap.h>
#include <Ludens/Media/Model.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderBackend/RStager.h>
#include <Ludens/RenderComponent/Layout/RMaterial.h>
#include <Ludens/Serial/Serial.h>

namespace LD {

struct RMeshPrimitive
{
    uint32_t indexStart;
    uint32_t indexCount;
    uint32_t matIndex;
};

/// @brief renderer friendly layout of a rigid mesh.
class RMesh
{
public:
    RDevice device{};
    RSetPool setPool;
    RBuffer vbo;
    RBuffer ibo;
    RImage* textures;
    RMaterial* mats;
    RMeshPrimitive* prims;
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t textureCount;
    uint32_t matCount;
    uint32_t primCount;

    inline operator bool() const { return (bool)device; }

    void create_from_media(RDevice device, RStager& stager, Model& model);
    void create_from_binary(RDevice device, RStager& stager, ModelBinary& bin);
    void destroy();

private:
    void upload(RStager& stager, uint32_t textureCount, const Bitmap* textureData,
                uint32_t matCount, const MeshMaterial* matData,
                uint32_t vertexCount, const MeshVertex* vertexData,
                uint32_t indexCount, const uint32_t* indexData);
};

} // namespace LD
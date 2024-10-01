#pragma once

#include <functional>
#include "Core/RenderBase/Include/RDevice.h"
#include "Core/RenderFX/Include/Groups/MaterialGroup.h"
#include "Core/Media/Include/Model.h"

namespace LD
{

struct RMeshInfo
{
    RDevice Device; // owner of this static mesh
    RBindingGroupLayout MaterialBGL;
    Ref<Model> Data = nullptr;
};

class RMesh
{
public:
    RMesh();
    RMesh(const RMesh&) = delete;
    ~RMesh();

    RMesh& operator=(const RMesh&) = delete;

    void Startup(const RMeshInfo& info);
    void Cleanup();

    struct Batch
    {
        RBuffer Vertices;    // batched vertex buffer
        RBuffer Indices;     // batched index buffer
        MaterialGroup Material; // material used throughout this batch
        u32 IndexCount;
        u32 VertexCount;
    };

    // called on each static mesh batch
    using BatchFn = std::function<void(Batch&)>;

    void Draw(BatchFn fn);

private:
    void PrepareMetallicRoughnessInfo(MaterialGroupInfo& matBGI, const Material& mat);

    RDevice mDevice;
    Vector<Batch> mBatches;
};

} // namespace LD
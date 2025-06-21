#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/Header/Math/Vec4.h>
#include <Ludens/RenderBackend/RBackend.h>

namespace LD {

/// @brief pipeline for rendering MeshVertex with Blinn-Phong lighting
struct RMeshBlinnPhongPipeline : Handle<struct RMeshBlinnPhongPipelineObj>
{
    struct PushConstant
    {
        Mat4 model;     /// model matrix transformation
        uint32_t id;    /// lower 16-bit written to the id-flag attachment
        uint32_t flags; /// lower 16-bit written to the id-flag attachment
    };

    static RMeshBlinnPhongPipeline create(RDevice device);
    static void destroy(RMeshBlinnPhongPipeline pipeline);

    RPipeline handle();
};

/// @brief pipeline for rendering MeshVertex with flat ambient lighting
struct RMeshAmbientPipeline : Handle<struct RMeshAmbientPipelineObj>
{
    struct PushConstant
    {
        Mat4 model;     /// model matrix transformation
        uint32_t id;    /// lower 16-bit written to the id-flag attachment
        uint32_t flags; /// lower 16-bit written to the id-flag attachment
        Vec4 ambient;   /// flat ambient color of the mesh
    };

    static RMeshAmbientPipeline create(RDevice device);
    static void destroy(RMeshAmbientPipeline pipeline);

    RPipeline handle();
};

} // namespace LD
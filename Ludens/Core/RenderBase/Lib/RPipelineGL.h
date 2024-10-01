#pragma once

#include <glad/glad.h>
#include "Core/DSA/Include/Vector.h"
#include "Core/RenderBase/Include/GL/GLVertexArray.h"
#include "Core/RenderBase/Include/GL/GLProgram.h"
#include "Core/RenderBase/Include/RPipeline.h"
#include "Core/RenderBase/Lib/RBase.h"

namespace LD
{

struct RDeviceGL;

struct RPipelineGL : RPipelineBase
{
    RPipelineGL();
    RPipelineGL(const RPipelineGL&) = delete;
    RPipelineGL(RPipelineGL&&) = default;
    ~RPipelineGL();

    RPipelineGL& operator=(const RPipelineGL&) = delete;
    RPipelineGL& operator=(RPipelineGL&&) noexcept = default;

    inline bool operator==(const RPipelineGL& other) const
    {
        return ID == other.ID;
    }

    inline bool operator!=(const RPipelineGL& other) const
    {
        return ID != other.ID;
    }

    void Startup(RPipeline& handle, const RPipelineInfo& spec, RDeviceGL& device);
    void Cleanup(RPipeline& handle);

    GLVertexArray VAO;
    GLProgram Program;
    GLenum GLPrimitiveTopology;
    GLenum GLPolygonMode;
    GLenum GLCullMode;
    GLenum GLColorBlendOp;
    GLenum GLColorBlendSrcFactor;
    GLenum GLColorBlendDstFactor;
    GLenum GLAlphaBlendOp;
    GLenum GLAlphaBlendSrcFactor;
    GLenum GLAlphaBlendDstFactor;
    bool BlendEnabled;
    Vector<u32> VertexStrides; // vertex byte size at each vertex buffer slot

    // for each binding group, map RTexture binding to OpenGL Texture Unit
    Vector<std::unordered_map<u32, u32>> TextureUnitBinding;

    // for each binding group, map uniform RBuffer binding to OpenGL Buffer Base
    Vector<std::unordered_map<u32, u32>> UniformBufferBinding;
};

} // namespace LD
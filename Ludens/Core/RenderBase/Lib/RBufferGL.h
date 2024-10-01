#pragma once

#include <glad/glad.h>
#include "Core/RenderBase/Include/GL/GLBuffer.h"
#include "Core/RenderBase/Include/RBuffer.h"
#include "Core/RenderBase/Include/RPipeline.h"
#include "Core/RenderBase/Include/RResult.h"
#include "Core/RenderBase/Lib/RBase.h"

namespace LD
{

struct RDeviceGL;

struct RBufferGL : RBufferBase
{
    RBufferGL();
    RBufferGL(const RBufferGL&) = delete;
    ~RBufferGL();

    RBufferGL& operator=(const RBufferGL&) = delete;

    void Startup(RBuffer& handle, const RBufferInfo& spec, RDeviceGL& device);
    void Cleanup(RBuffer& handle);
    void Bind();
    virtual RResult SetData(u32 offset, u32 size, const void* data) override;

    GLenum Target;

    union
    {
        GLVertexBuffer VBO;
        GLIndexBuffer IBO;
        GLUniformBuffer UBO;
    };
};

} // namespace LD
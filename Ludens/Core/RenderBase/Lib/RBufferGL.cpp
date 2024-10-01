#include <cstring>
#include "Core/RenderBase/Lib/RDeviceGL.h"
#include "Core/RenderBase/Lib/RBufferGL.h"
#include "Core/RenderBase/Lib/RDeriveGL.h"

namespace LD
{

RBufferGL::RBufferGL()
{
}

RBufferGL::~RBufferGL()
{
    LD_DEBUG_ASSERT(Device == nullptr);
}

void RBufferGL::Startup(RBuffer& handle, const RBufferInfo& info, RDeviceGL& device)
{
    RBufferBase::Startup(handle, info, &device);

    Target = DeriveGLBufferTarget(info.Type);

    switch (Target)
    {
    case GL_ARRAY_BUFFER:
    {
        GLVertexBufferInfo vboInfo{};
        vboInfo.Usage = GL_STATIC_DRAW;
        vboInfo.Data = info.Data;
        vboInfo.Size = info.Size;
        VBO.Startup(device.Context, vboInfo);
        break;
    }
    case GL_ELEMENT_ARRAY_BUFFER:
    {
        GLIndexBufferInfo iboInfo{};
        iboInfo.Usage = GL_STATIC_DRAW;
        iboInfo.Data = info.Data;
        iboInfo.Size = info.Size;
        IBO.Startup(device.Context, iboInfo);
        break;
    }
    case GL_UNIFORM_BUFFER:
    {
        GLUniformBufferInfo uboInfo{};
        uboInfo.Usage = GL_STATIC_DRAW;
        uboInfo.Size = info.Size;
        uboInfo.Data = info.Data;
        UBO.Startup(device.Context, uboInfo);
        break;
    }
    default:
        LD_DEBUG_UNREACHABLE;
    }
}

void RBufferGL::Cleanup(RBuffer& bufferH)
{
    RBufferBase::Cleanup(bufferH);

    switch (Target)
    {
    case GL_ARRAY_BUFFER:
        VBO.Cleanup();
        break;
    case GL_ELEMENT_ARRAY_BUFFER:
        IBO.Cleanup();
        break;
    case GL_UNIFORM_BUFFER:
        UBO.Cleanup();
        break;
    default:
        LD_DEBUG_UNREACHABLE;
    }
}

void RBufferGL::Bind()
{
    switch (Target)
    {
    case GL_ARRAY_BUFFER:
        VBO.Bind();
        break;
    case GL_ELEMENT_ARRAY_BUFFER:
        IBO.Bind();
        break;
    case GL_UNIFORM_BUFFER:
        UBO.Bind();
        break;
    default:
        LD_DEBUG_UNREACHABLE;
    }
}

RResult RBufferGL::SetData(u32 offset, u32 size, const void* data)
{
    Bind();

    switch (Target)
    {
    case GL_ARRAY_BUFFER:
        VBO.SetData(offset, size, data);
        break;
    case GL_UNIFORM_BUFFER:
        UBO.SetData(offset, size, data);
        break;
    default:
        LD_DEBUG_UNREACHABLE;
    }

    return {};
}

} // namespace LD
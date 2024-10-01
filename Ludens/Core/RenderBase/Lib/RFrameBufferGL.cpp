#include <algorithm>
#include <algorithm>
#include <glad/glad.h>
#include "Core/Header/Include/Error.h"
#include "Core/RenderBase/Lib/RFrameBufferGL.h"
#include "Core/RenderBase/Lib/RDeviceGL.h"
#include "Core/RenderBase/Lib/RDeriveGL.h"

namespace LD
{

RFrameBufferGL::RFrameBufferGL()
{
}

RFrameBufferGL::~RFrameBufferGL()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RFrameBufferGL::Startup(RFrameBuffer& frameBufferH, const RFrameBufferInfo& info, RDeviceGL& device)
{
    if (IsDefaultFrameBuffer)
    {
        frameBufferH.SetHandle(CUID<RFrameBufferBase>::Get(), this);
        return;
    }

    RFrameBufferBase::Startup(frameBufferH, info, (RDeviceBase*)&device);

    StartupGLAttachments();
}

void RFrameBufferGL::Cleanup(RFrameBuffer& frameBufferH)
{
    if (IsDefaultFrameBuffer)
    {
        frameBufferH.ResetHandle();
        return;
    }

    RFrameBufferBase::Cleanup(frameBufferH);

    CleanupGLAttachments();
}

void RFrameBufferGL::StartupGLAttachments()
{
    RDeviceGL* deviceGL = dynamic_cast<RDeviceGL*>(Device);

    Vector<GLTexture2D*> glColorAttachments(ColorAttachments.Size());
    for (size_t i = 0; i < glColorAttachments.Size(); i++)
    {
        RTextureGL& glTexture = Derive<RTextureGL>(ColorAttachments[i]);
        glColorAttachments[i] = &glTexture.Texture2D;
    }

    GLTexture2D* glDepthStencilAttachment = nullptr;
    if (DepthStencilAttachment.HasValue())
    {
        RTextureGL& glTexture = Derive<RTextureGL>(DepthStencilAttachment.Value());
        glDepthStencilAttachment = &glTexture.Texture2D;
    }

    GLFrameBufferInfo glInfo{};
    glInfo.Width = Width;
    glInfo.Height = Height;
    glInfo.ColorAttachmentCount = glColorAttachments.Size();
    glInfo.ColorAttachments = glColorAttachments.Data();
    glInfo.DepthStencilAttachment = glDepthStencilAttachment;
    FBO.Startup(deviceGL->Context, glInfo);
}

void RFrameBufferGL::CleanupGLAttachments()
{
    FBO.Cleanup();
}

RResult RFrameBufferGL::Invalidate(const RFrameBufferInfo& info)
{
    // NOTE: Calling Cleanup and Startup with the new info is feasible, but doing so will regenerate a UID for
    // RFrameBuffer handle.
    //       Here we are trying to preserve the original UID of the RFrameBuffer handle, only the OpenGL objects are
    //       recreated.

    CleanupGLAttachments();

    ReadInfo(info);

    StartupGLAttachments();

    return {};
}

} // namespace LD

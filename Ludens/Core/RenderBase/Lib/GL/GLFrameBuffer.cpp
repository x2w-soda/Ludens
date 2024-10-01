#include "Core/RenderBase/Include/GL/GLFrameBuffer.h"
#include "Core/RenderBase/Include/GL/GLContext.h"

namespace LD
{

GLFrameBuffer::GLFrameBuffer()
{
}

GLFrameBuffer::~GLFrameBuffer()
{
    LD_DEBUG_ASSERT(mHandle == 0);
}

void GLFrameBuffer::Startup(GLContext& context, const GLFrameBufferInfo& info)
{
    mContext = &context;
    mHandle = CUID<GLFrameBuffer>::Get();
    mInfo = info;

    glCreateFramebuffers(1, &mFrameBuffer);
    mContext->BindFrameBuffer(*this);

    mColorAttachmentCount = info.ColorAttachmentCount;

    for (size_t i = 0; i < mColorAttachmentCount; i++)
    {
        GLTexture2D& attachment = *info.ColorAttachments[i];

        attachment.Bind(0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, (GLuint)attachment, 0);
    }

    if (info.DepthStencilAttachment)
    {
        GLTexture2D& attachment = *info.DepthStencilAttachment;
        GLenum format = attachment.GetDataFormat();
        GLenum attachmentType;

        if (format == GL_DEPTH_COMPONENT)
        {
            attachmentType = GL_DEPTH_ATTACHMENT;
            mHasDepthBits = true;
            mHasStencilBits = false;
        }
        else if (format == GL_DEPTH_STENCIL)
        {
            attachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
            mHasDepthBits = true;
            mHasStencilBits = true;
        }
        else
            LD_DEBUG_UNREACHABLE;

        attachment.Bind(0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D, (GLuint)attachment, 0);
    }
    else
    {
        mHasDepthBits = false;
        mHasStencilBits = false;
    }

    LD_DEBUG_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    mContext->UnbindFrameBuffer();
}

void GLFrameBuffer::Cleanup()
{
    mHandle.Reset();
    mContext = nullptr;

    glDeleteFramebuffers(1, &mFrameBuffer);
}

void GLFrameBuffer::Bind()
{
    mContext->BindFrameBuffer(*this);
}

void GLFrameBuffer::BindDrawBuffers()
{
    Vector<GLenum> drawBuffers(mColorAttachmentCount);

    for (size_t i = 0; i < mColorAttachmentCount; i++)
    {
        drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
    }

    glDrawBuffers(drawBuffers.Size(), drawBuffers.Data());
}

} // namespace LD
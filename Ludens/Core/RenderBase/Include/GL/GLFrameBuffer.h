#pragma once

#include <glad/glad.h>
#include "Core/OS/Include/UID.h"
#include "Core/DSA/Include/Vector.h"
#include "Core/DSA/Include/Array.h"
#include "Core/DSA/Include/Optional.h"
#include "Core/Header/Include/Types.h"
#include "Core/RenderBase/Include/GL/GLTexture.h"

namespace LD
{

class GLContext;

struct GLAttachmentInfo
{
    GLenum InternalFormat;
    GLenum DataFormat;
    GLenum Type;
};

struct GLFrameBufferInfo
{
    u16 Width = 0;
    u16 Height = 0;
    size_t ColorAttachmentCount = 0;
    GLTexture2D** ColorAttachments = nullptr;
    GLTexture2D* DepthStencilAttachment = nullptr;
};

class GLFrameBuffer
{
public:
    GLFrameBuffer();
    GLFrameBuffer(const GLFrameBuffer&) = delete;
    ~GLFrameBuffer();

    GLFrameBuffer& operator=(const GLFrameBuffer&) = delete;

    void Startup(GLContext& context, const GLFrameBufferInfo& info);
    void Cleanup();
    void Bind();
    void BindDrawBuffers();

    inline bool HasDepthBits() const
    {
        return mHasDepthBits;
    }
    
    inline bool HasStencilBits() const
    {
        return mHasStencilBits;
    }

    inline UID GetHandle() const
    {
        return (UID)mHandle;
    }
    
    inline explicit operator UID() const
    {
        return (UID)mHandle;
    }

    inline explicit operator GLuint() const
    {
        return mFrameBuffer;
    }

private:
    CUID<GLFrameBuffer> mHandle;
    GLContext* mContext = nullptr;
    GLFrameBufferInfo mInfo;
    GLuint mFrameBuffer;
    size_t mColorAttachmentCount = 0;
    bool mHasDepthBits;
    bool mHasStencilBits;
};

} // namespace LD
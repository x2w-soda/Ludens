#pragma once

#include <glad/glad.h>
#include "Core/OS/Include/UID.h"

namespace LD
{

class GLContext;
class GLIndexBuffer;

class GLVertexArray
{
public:
    GLVertexArray();
    GLVertexArray(const GLVertexArray&) = delete;
    ~GLVertexArray();

    GLVertexArray& operator=(const GLVertexArray&) = delete;

    void Startup(GLContext& context);
    void Cleanup();

    void Bind();
    void BindIBO(GLIndexBuffer& ibo);

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
        return mVAO;
    }

private:
    CUID<GLVertexArray> mHandle;
    GLContext* mContext = nullptr;
    GLuint mVAO;
    UID mBoundIBO = 0;
};

} // namespace LD
#include <iostream>
#include "Core/RenderBase/Include/GL/GLVertexArray.h"
#include "Core/RenderBase/Include/GL/GLContext.h"
#include "Core/RenderBase/Include/GL/GLBuffer.h"
#include "Core/Header/Include/Error.h"

namespace LD
{

GLVertexArray::GLVertexArray() : mContext(nullptr)
{
}

GLVertexArray::~GLVertexArray()
{
    LD_DEBUG_ASSERT(mContext == nullptr);
}

void GLVertexArray::Startup(GLContext& context)
{
    mHandle = CUID<GLVertexArray>::Get();
    mContext = &context;

    glCreateVertexArrays(1, &mVAO);
}

void GLVertexArray::Cleanup()
{
    GLVertexArray* boundVAO = mContext->GetBoundVAO();

    if (boundVAO == this)
    {
        mContext->BindVAO(nullptr);
    }

    glDeleteVertexArrays(1, &mVAO);

    mHandle.Reset();
    mContext = nullptr;
}

void GLVertexArray::Bind()
{
    LD_DEBUG_ASSERT(mContext != nullptr);

    mContext->BindVAO(this);
}

void GLVertexArray::BindIBO(GLIndexBuffer& ibo)
{
    if (mBoundIBO == (UID)ibo)
    {
        LD_DEBUG_ASSERT(
            [&]()
            {
                GLint actual;
                glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &actual);
                return actual == (GLuint)ibo;
            }());
        return;
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)ibo);
    mBoundIBO = (UID)ibo;
}

} // namespace LD
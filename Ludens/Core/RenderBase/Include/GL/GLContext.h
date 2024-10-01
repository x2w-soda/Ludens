#pragma once

#include <set>
#include <string>
#include <glad/glad.h>
#include "Core/OS/Include/UID.h"

namespace LD
{

class GLVertexArray;
class GLVertexBuffer;
class GLIndexBuffer;
class GLUniformBuffer;
class GLTexture2D;
class GLTexture2DArray;
class GLTextureCube;
class GLFrameBuffer;
class GLProgram;

struct GLContextLimits
{
    std::string ToString() const;

    int MaxTextureImageUnits;
    int MaxCombinedTextureImageUnits;
    int MaxElementIndex;
    int MaxDrawBuffers;
    int MaxColorAttachments;
    int MaxUniformBufferBindings;
    int MaxUniformBlockSize;
};

// currently not enforced as a singleton class, but the general use case
// involves using a single context which out-lives all other GL resources.
class GLContext
{
public:
    GLContext();
    GLContext(const GLContext&) = delete;
    ~GLContext();

    GLContext& operator=(const GLContext&) = delete;

    void Startup();
    void Cleanup();

    bool HasExtension(const char* name);

    void BindVAO(GLVertexArray* vao);
    void BindVBO(GLVertexBuffer& vbo);
    void BindIBO(GLIndexBuffer& ibo);
    void BindUBO(GLUniformBuffer& ubo);
    void BindTextureUnit(int unit);
    void BindTexture2D(GLTexture2D& texture);
    void BindTexture2DArray(GLTexture2DArray& textureArray);
    void BindTextureCube(GLTextureCube& textureCube);
    void BindProgram(GLProgram& program);
    void BindFrameBuffer(GLFrameBuffer& frameBuffer);
    void UnbindProgram();
    void UnbindFrameBuffer();

    inline GLuint GetVersion() const
    {
        return mVersion;
    }

    inline void GetDefaultFrameBufferDepth(GLint* bits, GLint* type) const
    {
        *bits = mDefaultFrameBufferDepthBits;
        *type = mDefaultFrameBufferDepthType;
    }

    inline void GetDefaultFrameBufferStencil(GLint* bits, GLint* type) const
    {
        *bits = mDefaultFrameBufferStencilBits;
        *type = mDefaultFrameBufferStencilType;
    }

    inline const GLContextLimits& GetLimits() const
    {
        return sLimits;
    }

    inline GLVertexArray* GetBoundVAO() const
    {
        return mBoundVAO;
    }

    inline UID GetBoundProgram() const
    {
        return mBoundProgram;
    }

private:
    void QueryLimits();

    static bool sHasGladInit;
    static GLContextLimits sLimits;

    std::set<std::string> mExtensions;
    GLuint mVersion;
    GLVertexArray* mBoundVAO = nullptr;
    UID mBoundVBO = 0;
    UID mBoundUBO = 0;
    UID mBoundProgram = 0;
    UID mBoundTexture2D = 0;
    UID mBoundTexture2DArray = 0;
    UID mBoundTextureCube = 0;
    UID mBoundFrameBuffer = 0;
    int mBoundTextureUnit = 0;
    GLint mDefaultFrameBufferDepthBits;
    GLint mDefaultFrameBufferStencilBits;
    GLint mDefaultFrameBufferDepthType;
    GLint mDefaultFrameBufferStencilType;
};

} // namespace LD
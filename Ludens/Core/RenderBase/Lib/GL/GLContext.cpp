#include <iostream>
#include <sstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Core/RenderBase/Include/GL/GLContext.h"
#include "Core/RenderBase/Include/GL/GLVertexArray.h"
#include "Core/RenderBase/Include/GL/GLBuffer.h"
#include "Core/RenderBase/Include/GL/GLTexture.h"
#include "Core/RenderBase/Include/GL/GLProgram.h"
#include "Core/RenderBase/Include/GL/GLFrameBuffer.h"
#include "Core/Header/Include/Error.h"

namespace LD
{

bool GLContext::sHasGladInit = false;
GLContextLimits GLContext::sLimits;

GLContext::GLContext()
{
}

GLContext::~GLContext()
{
}

void GLContext::Startup()
{
    // lazy initialization in first GLContext setup
    if (!sHasGladInit)
    {
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cout << "gladLoadGLLoader failed" << std::endl;
            return;
        }

        QueryLimits();
        sHasGladInit = true;
    }

    LD_DEBUG_ASSERT(sHasGladInit);

    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    mVersion = (GLuint)(major * 100 + minor * 10);

    GLint numExtensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    for (GLint i = 0; i < numExtensions; ++i)
    {
        const char* extensionName = (const char*)glGetStringi(GL_EXTENSIONS, i);
        mExtensions.insert(extensionName);
    }

    // TODO: relax this requirement, currently RShaderGL implementation
    //       consumes pre-compiled SPIRV shaders only.
    LD_DEBUG_ASSERT(HasExtension("GL_ARB_gl_spirv"));

    UnbindFrameBuffer();
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE,
                                          &mDefaultFrameBufferDepthBits);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE,
                                          &mDefaultFrameBufferStencilBits);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE,
                                          &mDefaultFrameBufferDepthType);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE,
                                          &mDefaultFrameBufferStencilType);

    glActiveTexture(GL_TEXTURE0);
    mBoundTextureUnit = 0;
}

void GLContext::Cleanup()
{
}

bool GLContext::HasExtension(const char* name)
{
    return mExtensions.find(name) != mExtensions.end();
}

void GLContext::BindVAO(GLVertexArray* vao)
{
    if (vao == nullptr)
    {
        mBoundVAO = nullptr;
        return;
    }

    if (mBoundVAO != nullptr && (UID)*mBoundVAO == (UID)*vao)
    {
        LD_DEBUG_ASSERT(
            [&]()
            {
                GLint actual;
                glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &actual);
                return actual == (GLuint)*vao;
            }());
        return;
    }

    glBindVertexArray((GLuint)*vao);
    mBoundVAO = vao;
}

void GLContext::BindVBO(GLVertexBuffer& vbo)
{
    if (mBoundVBO == (UID)vbo)
    {
        LD_DEBUG_ASSERT(
            [&]()
            {
                GLint actual;
                glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &actual);
                return actual == (GLuint)vbo;
            }());
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)vbo);
    mBoundVBO = (UID)vbo;
}

void GLContext::BindIBO(GLIndexBuffer& ibo)
{
    LD_DEBUG_ASSERT(mBoundVAO != nullptr && "binding IBO without a previously bound VAO");

    mBoundVAO->BindIBO(ibo);
}

void GLContext::BindUBO(GLUniformBuffer& ubo)
{
    if (mBoundUBO == (UID)ubo)
    {
        LD_DEBUG_ASSERT(
            [&]()
            {
                GLint actual;
                glGetIntegerv(GL_UNIFORM_BUFFER_BINDING, &actual);
                return actual == (GLuint)ubo;
            }());
        return;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, (GLuint)ubo);
    mBoundUBO = (UID)ubo;
}

void GLContext::BindTextureUnit(int unit)
{
    if (mBoundTextureUnit == unit)
    {
        LD_DEBUG_ASSERT(
            [&]()
            {
                GLint actual;
                glGetIntegerv(GL_ACTIVE_TEXTURE, &actual);
                return actual == (GL_TEXTURE0 + unit);
            }());
        return;
    }

    glActiveTexture(GL_TEXTURE0 + unit);
    mBoundTextureUnit = unit;
}

void GLContext::BindTexture2D(GLTexture2D& texture)
{
    // NOTE: OpenGL texture units supports bindings to all targets, here we don't cache which texture unit we bind to,
    //       only the last texture bound for each texture target.
    glBindTexture(GL_TEXTURE_2D, (GLuint)texture);
    mBoundTexture2D = (UID)texture;
}

void GLContext::BindTexture2DArray(GLTexture2DArray& texture)
{
    glBindTexture(GL_TEXTURE_2D_ARRAY, (GLuint)texture);
    mBoundTexture2DArray = (UID)texture;
}

void GLContext::BindTextureCube(GLTextureCube& textureCube)
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, (GLuint)textureCube);
    mBoundTextureCube = (UID)textureCube;
}

void GLContext::BindProgram(GLProgram& shader)
{
    if (mBoundProgram == (UID)shader)
    {
        LD_DEBUG_ASSERT(
            [&]()
            {
                GLint actual;
                glGetIntegerv(GL_CURRENT_PROGRAM, &actual);
                return actual == (GLuint)shader;
            }());
        return;
    }
    glUseProgram((GLuint)shader);
    mBoundProgram = (UID)shader;
}

void GLContext::BindFrameBuffer(GLFrameBuffer& frameBuffer)
{
    if (mBoundFrameBuffer == (UID)frameBuffer)
    {
        LD_DEBUG_ASSERT(
            [&]()
            {
                GLint actual;
                glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &actual);
                return actual == (GLuint)frameBuffer;
            }());
        LD_DEBUG_ASSERT(
            [&]()
            {
                GLint actual;
                glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &actual);
                return actual == (GLuint)frameBuffer;
            }());
        return;
    }

    // NOTE: currently only binds to GL_FRAMEBUFFER target, so GL_DRAW_FRAMEBUFFER and GL_READ_FRAMEBUFFER should always
    // be the same.
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)frameBuffer);
    mBoundFrameBuffer = (UID)frameBuffer;
}

void GLContext::UnbindProgram()
{
    glUseProgram(0);
    mBoundProgram = 0;
}

void GLContext::UnbindFrameBuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    mBoundFrameBuffer = 0;
}

void GLContext::QueryLimits()
{
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &sLimits.MaxTextureImageUnits);
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &sLimits.MaxCombinedTextureImageUnits);
    glGetIntegerv(GL_MAX_ELEMENT_INDEX, &sLimits.MaxElementIndex);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &sLimits.MaxDrawBuffers);
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &sLimits.MaxColorAttachments);
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &sLimits.MaxUniformBlockSize);
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &sLimits.MaxUniformBufferBindings);
}

std::string GLContextLimits::ToString() const
{
    std::stringstream ss;

    ss << "GL_MAX_TEXTURE_IMAGE_UNITS:\t" << MaxTextureImageUnits << '\n';
    ss << "GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:\t" << MaxCombinedTextureImageUnits << '\n';
    ss << "GL_MAX_ELEMENT_INDEX:\t" << MaxTextureImageUnits << '\n';
    ss << "GL_MAX_DRAW_BUFFERS:\t" << MaxDrawBuffers << '\n';
    ss << "GL_MAX_COLOR_ATTACHMENTS:\t" << MaxColorAttachments << '\n';
    ss << "GL_MAX_UNIFORM_BLOCK_SIZE:\t" << MaxUniformBlockSize << '\n';
    ss << "GL_MAX_UNIFORM_BUFFER_BINDINGS:\t" << MaxUniformBufferBindings << '\n';

    return ss.str();
}

} // namespace LD
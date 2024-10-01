#include "Core/Header/Include/Error.h"
#include "Core/RenderBase/Include/GL/GLCommand.h"

namespace LD
{
namespace GLCommand
{

void DrawArrays(GLenum primitive, u32 vertexCount)
{
    glDrawArrays(primitive, 0, vertexCount);
}

void DrawArraysInstanced(GLenum primitive, u32 vertexCount, GLsizei instanceCount)
{
    glDrawArraysInstanced(primitive, 0, vertexCount, instanceCount);
}

void DrawArraysInstanced(GLenum primitive, u32 vertexCount, GLsizei instanceCount, GLuint instanceStart)
{
    glDrawArraysInstancedBaseInstance(primitive, 0, vertexCount, instanceCount, instanceStart);
}

void DrawElements(GLenum primitive, u32 indexCount, GLenum indexType)
{
    glDrawElements(primitive, indexCount, indexType, nullptr);
}

void DrawElementsInstanced(GLenum primitive, u32 indexCount, GLenum indexType, GLsizei instanceCount)
{
    glDrawElementsInstanced(primitive, indexCount, indexType, nullptr, instanceCount);
}

void DrawElementsInstanced(GLenum primitive, u32 indexCount, GLenum indexType, GLsizei instanceCount,
                           GLuint instanceStart)
{
    // NOTE: the instanceStart offset is *NOT* applied to the gl_InstanceID variable in the vertex shader GLSL,
    //       use the gl_InstanceIndex variable provided by GL_KHR_vulkan_glsl instead.
    glDrawElementsInstancedBaseInstance(primitive, indexCount, indexType, nullptr, instanceCount, instanceStart);
}

void DrawElementsInstanced(GLenum primitive, u32 indexCount, GLenum indexType, GLsizei instanceCount,
                           GLuint indexStart, GLuint instanceStart)
{
    LD_DEBUG_ASSERT(indexType == GL_UNSIGNED_INT || indexType == GL_UNSIGNED_SHORT);
    size_t indexSize = indexType == GL_UNSIGNED_INT ? 4 : 2;
    size_t offset = indexStart * indexSize;
    glDrawElementsInstancedBaseInstance(primitive, indexCount, indexType, (void*)offset, instanceCount, instanceStart);
}

} // namespace GLCommand
} // namespace LD
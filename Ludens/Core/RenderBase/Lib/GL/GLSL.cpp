#include "Core/RenderBase/Include/GL/GLSL.h"
#include "Core/Header/Include/Error.h"

namespace LD
{

bool GetGLSLTypeVertexAttribute(GLSLType glslType, GLint* componentCount, GLenum* componentType, u32* byteSize)
{
    GLint count;
    GLenum type;
    u32 size;

    switch (glslType)
    {
    case GLSLType::Float:
        count = 1;
        type = GL_FLOAT;
        size = 4;
        break;
    case GLSLType::Vec2:
        count = 2;
        type = GL_FLOAT;
        size = 8;
        break;
    case GLSLType::Vec3:
        count = 3;
        type = GL_FLOAT;
        size = 12;
        break;
    case GLSLType::Vec4:
        count = 4;
        type = GL_FLOAT;
        size = 16;
        break;
    default:
        return false;
    }

    if (componentCount)
        *componentCount = count;
    if (componentType)
        *componentType = type;
    if (byteSize)
        *byteSize = size;

    return true;
}

} // namespace LD
#pragma once

#include <cstdint>
#include <glad/glad.h>
#include "Core/Header/Include/Types.h"

namespace LD
{

enum class GLSLType
{
    Float = 0,
    Vec2,
    Vec3,
    Vec4,
};

bool GetGLSLTypeVertexAttribute(GLSLType type, GLint* componentCount, GLenum* componentType, u32* byteSize);

} // namespace LD
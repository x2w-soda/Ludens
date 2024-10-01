#include <iostream>
#include "Core/Header/Include/Types.h"
#include "Core/RenderBase/Include/GL/GLVertex.h"

namespace LD
{

GLVertexLayout& GLVertexLayout::AddVertexAttribute(const GLVertexAttribute& attribute)
{
    uint32_t attrByteSize;
    GLint attrComponentCount;
    GLenum attrComponentType;

    GetGLSLTypeVertexAttribute(attribute.Type, &attrComponentCount, &attrComponentType, &attrByteSize);

    mOffsets.PushBack(mVertexStride);
    mVertexStride += attrByteSize;
    mAttributes.PushBack(attribute);

    return *this;
}

} // namespace LD
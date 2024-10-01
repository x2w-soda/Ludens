#include "Core/Header/Include/Error.h"
#include "Core/RenderBase/Lib/RDeriveGL.h"
#include "Core/RenderBase/Include/GL/GLFrameBuffer.h"
#include "Core/RenderBase/Include/RFrameBuffer.h"

namespace LD
{

struct GLTextureFormat
{
    GLenum InternalFormat;
    GLenum DataFormat;
    GLenum DataType;
};

// clang-format off
static const GLTextureFormat sGLTextureFormats[] = {
    { GL_ZERO,                  GL_ZERO,               GL_ZERO },              // Undefined
    { GL_R8,                    GL_RED,                GL_UNSIGNED_BYTE },     // R8
    { GL_RGBA8,                 GL_BGRA,               GL_UNSIGNED_BYTE },     // BGRA8
    { GL_RGBA8,                 GL_RGBA,               GL_UNSIGNED_BYTE },     // RGBA8
    { GL_RGBA16F,               GL_RGBA,               GL_HALF_FLOAT },        // RGBA16F
    { GL_DEPTH24_STENCIL8,      GL_DEPTH_STENCIL,      GL_UNSIGNED_INT_24_8 }, // D24S8
    { GL_DEPTH_COMPONENT32F,    GL_DEPTH_COMPONENT,    GL_FLOAT },             // D32F
};
// clang-format on

LD_STATIC_ASSERT(sizeof(sGLTextureFormats) / sizeof(GLTextureFormat) == (size_t)RTextureFormat::EnumCount);

void DeriveGLTextureFormat(const RTextureFormat& format, GLenum* internalFormat, GLenum* dataFormat, GLenum* dataType)
{
    const GLTextureFormat& glFormat = sGLTextureFormats[(int)format];

    if (internalFormat)
        *internalFormat = glFormat.InternalFormat;

    if (dataFormat)
        *dataFormat = glFormat.DataFormat;

    if (dataType)
        *dataType = glFormat.DataType;
}

void DeriveGLSamplerFilter(const RSamplerFilter& inFilter, GLenum& outFilter)
{
    switch (inFilter)
    {
    case RSamplerFilter::Linear:
        outFilter = GL_LINEAR;
        break;
    case RSamplerFilter::Nearest:
        outFilter = GL_NEAREST;
        break;
    default:
        LD_DEBUG_UNREACHABLE;
    }
}

void DeriveGLSamplerAddressMode(const RSamplerAddressMode& inAddrMode, GLenum& outAddrMode)
{
    switch (inAddrMode)
    {
    case RSamplerAddressMode::Repeat:
        outAddrMode = GL_REPEAT;
        break;
    case RSamplerAddressMode::ClampToEdge:
        outAddrMode = GL_CLAMP_TO_EDGE;
        break;
    default:
        LD_DEBUG_UNREACHABLE;
    }
}

} // namespace LD
#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <glad/glad.h> // hide

namespace LD {
namespace RUtil {

void cast_glsl_type_gl(const GLSLType& inType, GLint& outComponentCount, GLenum& outComponentType);
void cast_format_gl(const RFormat& inFormat, GLenum& outInternalFormat, GLenum& outDataFormat, GLenum& outDataType);
void cast_shader_type_gl(const RShaderType& inType, GLenum& outType);
void cast_image_type_gl(const RImageType& inType, GLenum& outTarget);
void cast_filter_gl(const RSamplerInfo& inSampler, GLenum& outMinFilter, GLenum& outMagFilter);
void cast_sampler_address_mode_gl(const RSamplerAddressMode& inMode, GLenum& outMode);
void cast_primitive_topology_gl(const RPrimitiveTopology& inTopo, GLenum& outTopo);

} // namespace RUtil
} // namespace LD
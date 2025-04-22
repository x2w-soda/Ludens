#pragma once

#include <cstdint>

namespace LD {

/// @brief render device backend
enum RDeviceBackend
{
    RDEVICE_BACKEND_VULKAN = 0,
};

enum RFormat
{
    RFORMAT_UNDEFINED = 0,
    RFORMAT_R8,
    RFORMAT_BGRA8,
    RFORMAT_RGBA8,
};

enum RFilter
{
    RFILTER_NEAREST = 0,
    RFILTER_LINEAR,
};

enum RSamplerAddressMode
{
    RSAMPLER_ADDRESS_MODE_REPEAT = 0,
    RSAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
    RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
};

enum RImageLayout
{
    RIMAGE_LAYOUT_UNDEFINED = 0,
    RIMAGE_LAYOUT_GENERAL,
    RIMAGE_LAYOUT_COLOR_ATTACHMENT,
    RIMAGE_LAYOUT_PRESENT_SRC,
    RIMAGE_LAYOUT_SHADER_READ_ONLY,
    RIMAGE_LAYOUT_TRANSFER_SRC,
    RIMAGE_LAYOUT_TRANSFER_DST,
};

enum RAttachmentLoadOp
{
    RATTACHMENT_LOAD_OP_LOAD = 0,
    RATTACHMENT_LOAD_OP_CLEAR,
    RATTACHMENT_LOAD_OP_DONT_CARE,
};

enum RAttachmentStoreOp
{
    RATTACHMENT_STORE_OP_STORE = 0,
    RATTACHMENT_STORE_OP_DONT_CARE,
};

enum RBindingType
{
    RBINDING_TYPE_COMBINED_IMAGE_SAMPLER = 0,
    RBINDING_TYPE_STORAGE_IMAGE,
    RBINDING_TYPE_UNIFORM_BUFFER,
};

enum RBindingInputRate
{
    RBINDING_INPUT_RATE_VERTEX = 0, /// new attributes are polled for each vertex (gl_VertexIndex)
    RBINDING_INPUT_RATE_INSTANCE,   /// new attributes are polled for each instance (gl_VertexInstance)
};

using RBufferUsageFlags = uint32_t;
enum RBufferUsageBit : RBufferUsageFlags
{
    RBUFFER_USAGE_TRANSFER_SRC_BIT = 1, /// src buffer for transfer commands
    RBUFFER_USAGE_TRANSFER_DST_BIT = 2, /// dst buffer for transfer commands
    RBUFFER_USAGE_VERTEX_BIT = 4,       /// vertex buffer (VBO) usage
    RBUFFER_USAGE_INDEX_BIT = 8,        /// index buffer (IBO) usage
    RBUFFER_USAGE_UNIFORM_BIT = 16      /// uniform buffer (UBO) usage
};

enum RImageType
{
    RIMAGE_TYPE_2D = 0,
};

using RImageUsageFlags = uint32_t;
enum RImageUsageBit : RImageUsageFlags
{
    RIMAGE_USAGE_TRANSFER_SRC_BIT = 1, // src image for transfer commands
    RIMAGE_USAGE_TRANSFER_DST_BIT = 2, // dst image for transfer commands
    RIMAGE_USAGE_SAMPLED_BIT = 4,
    RIMAGE_USAGE_STORAGE_BIT = 8,
    RIMAGE_USAGE_COLOR_ATTACHMENT_BIT = 16,
};

enum RShaderType
{
    RSHADER_TYPE_COMPUTE = 0,
    RSHADER_TYPE_VERTEX,
    RSHADER_TYPE_FRAGMENT,
};

using RPipelineStageFlags = uint32_t;
enum RPipelineStageBit : RPipelineStageFlags
{
    RPIPELINE_STAGE_TOP_OF_PIPE_BIT = 1,
    RPIPELINE_STAGE_DRAW_INDIRECT_BIT = 2,
    RPIPELINE_STAGE_VERTEX_SHADER_BIT = 4,
    RPIPELINE_STAGE_FRAGMENT_SHADER_BIT = 8,
    RPIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT = 16,
    RPIPELINE_STAGE_COMPUTE_SHADER_BIT = 32,
    RPIPELINE_STAGE_TRANSFER_BIT = 64,
    RPIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = 128,
    RPIPELINE_STAGE_BITS_ENUM_LAST_BIT = 256,
};

using RAccessFlags = uint32_t;
enum RAccessBit : RAccessFlags
{
    RACCESS_INDIRECT_COMMAND_READ_BIT = 1,
    RACCESS_INDEX_READ_BIT = 2,
    RACCESS_SHADER_READ_BIT = 4,
    RACCESS_SHADER_WRITE_BIT = 8,
    RACCESS_COLOR_ATTACHMENT_READ_BIT = 16,
    RACCESS_COLOR_ATTACHMENT_WRITE_BIT = 32,
    RACCESS_TRANSFER_READ_BIT = 64,
    RACCESS_TRANSFER_WRITE_BIT = 128,
    RACCESS_BITS_ENUM_LAST_BIT = 256,
};

enum RPolygonMode
{
    RPOLYGON_MODE_FILL = 0,
    RPOLYGON_MODE_LINE,
    RPOLYGON_MODE_POINT,
};

enum RCullMode
{
    RCULL_MODE_NONE = 0,
    RCULL_MODE_FRONT,
    RCULL_MODE_BACK,
};

enum RBlendFactor
{
    RBLEND_FACTOR_ZERO = 0,
    RBLEND_FACTOR_ONE,
    RBLEND_FACTOR_SRC_ALPHA,
    RBLEND_FACTOR_DST_ALPHA,
    RBLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    RBLEND_FACTOR_ONE_MINUS_DST_ALPHA,
};

enum RBlendOp
{
    RBLEND_OP_ADD = 0,
    RBLEND_OP_SUBTRACT,
    RBLEND_OP_REVERSE_SUBTRACT,
    RBLEND_OP_MIN,
    RBLEND_OP_MAX,
};

/// @brief bit size of unsigned integer indices
enum RIndexType
{
    RINDEX_TYPE_U16 = 0,
    RINDEX_TYPE_U32,
};

enum RGLSLType
{
    RGLSL_TYPE_FLOAT = 0,
    RGLSL_TYPE_VEC2,
    RGLSL_TYPE_VEC3,
    RGLSL_TYPE_VEC4,
    RGLSL_TYPE_UINT,
};

} // namespace LD
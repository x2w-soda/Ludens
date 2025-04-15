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
    RFORMAT_BGRA8,
    RFORMAT_RGBA8,
};

enum RImageLayout
{
    RIMAGE_LAYOUT_UNDEFINED = 0,
    RIMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    RIMAGE_LAYOUT_PRESENT_SRC,
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
};

enum RShaderType
{
    RSHADER_TYPE_VERTEX_SHADER = 0,
    RSHADER_TYPE_FRAGMENT_SHADER,
};

using RPipelineStageFlags = uint32_t;
enum RPipelineStageBit : RPipelineStageFlags
{
    RPIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT = 1,
    RPIPELINE_STAGE_BITS_ENUM_LAST_BIT = 2,
};

using RAccessFlags = uint32_t;
enum RAccessBit : RAccessFlags
{
    RACCESS_COLOR_ATTACHMENT_READ_BIT = 1,
    RACCESS_COLOR_ATTACHMENT_WRITE_BIT = 2,
    RACCESS_BITS_ENUM_LAST_BIT = 4,
};

enum RGLSLType
{
    RGLSL_TYPE_FLOAT = 0,
    RGLSL_TYPE_VEC2,
    RGLSL_TYPE_VEC3,
    RGLSL_TYPE_VEC4,
};

} // namespace LD
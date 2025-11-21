#pragma once

#include <Ludens/RenderBackend/RBackend.h>

#include <cstdint>
#include <vector>

#include "RData.h"

namespace LD {

struct RFramebufferObj;

enum RCommandType
{
    RCOMMAND_BEGIN_PASS,
    RCOMMAND_PUSH_CONSTANT,
    RCOMMAND_BIND_GRAPHICS_PIPELINE,
    RCOMMAND_BIND_GRAPHICS_SETS,
    RCOMMAND_BIND_COMPUTE_PIPELINE,
    RCOMMAND_BIND_COMPUTE_SETS,
    RCOMMAND_BIND_VERTEX_BUFFERS,
    RCOMMAND_BIND_INDEX_BUFFER,
    RCOMMAND_SET_SCISSOR,
    RCOMMAND_DRAW,
    RCOMMAND_DRAW_INDEXED,
    RCOMMAND_DRAW_INDIRECT,
    RCOMMAND_DRAW_INDEXED_INDIRECT,
    RCOMMAND_END_PASS,
    RCOMMAND_DISPATCH,
    RCOMMAND_BUFFER_MEMORY_BARRIER,
    RCOMMAND_IMAGE_MEMORY_BARRIER,
    RCOMMAND_COPY_BUFFER,
    RCOMMAND_COPY_BUFFER_TO_IMAGE,
    RCOMMAND_COPY_IMAGE_TO_BUFFER,
    RCOMMAND_BLIT_IMAGE,
    RCOMMAND_TYPE_ENUM_COUNT,
};

/// @brief Capture of RCOMMAND_BEGIN_PASS.
struct RCommandBeginPass
{
    const RCommandType type = RCOMMAND_BEGIN_PASS;
    uint32_t width;
    uint32_t height;
    RImage depthStencilAttachment;
    std::vector<RImage> colorAttachments;
    std::vector<RImage> colorResolveAttachments;
    std::vector<RClearColorValue> clearColors;
    RClearDepthStencilValue clearDepthStencil;
    RPassInfoData pass;
    RFramebufferObj* framebufferObj;

    RCommandBeginPass() = delete;
    RCommandBeginPass(const RPassBeginInfo& passBI, RFramebufferObj* framebufferObj);
};

/// @brief Capture of RCOMMAND_PUSH_CONSTANT.
struct RCommandPushConstant
{
    const RCommandType type = RCOMMAND_PUSH_CONSTANT;
    uint32_t offset;
    uint32_t size;
    const void* data;

    RCommandPushConstant() = delete;
    RCommandPushConstant(uint32_t offset, uint32_t size, const void* data);
};

/// @brief Capture of RCOMMAND_BIND_GRAPHICS_PIPELINE.
struct RCommandBindGraphicsPipeline
{
    const RCommandType type = RCOMMAND_BIND_GRAPHICS_PIPELINE;
    RPipeline pipeline;

    RCommandBindGraphicsPipeline() = delete;
    RCommandBindGraphicsPipeline(RPipeline pipeline);
};

/// @brief Capture of RCOMMAND_BIND_COMPUTE_PIPELINE.
struct RCommandBindComputePipeline
{
    const RCommandType type = RCOMMAND_BIND_COMPUTE_PIPELINE;
    RPipeline pipeline;

    RCommandBindComputePipeline() = delete;
    RCommandBindComputePipeline(RPipeline pipeline);
};

/// @brief Capture of RCOMMAND_BIND_GRAPHICS_SETS.
struct RCommandBindGraphicsSets
{
    const RCommandType type = RCOMMAND_BIND_GRAPHICS_SETS;
    uint32_t firstSet;
    std::vector<RSet> sets;

    RCommandBindGraphicsSets() = delete;
    RCommandBindGraphicsSets(uint32_t firstSet, uint32_t setCount, RSet* sets);
};

/// @brief Capture of RCOMMAND_BIND_COMPUTE_SETS.
struct RCommandBindComputeSets
{
    const RCommandType type = RCOMMAND_BIND_COMPUTE_SETS;
    uint32_t firstSet;
    std::vector<RSet> sets;

    RCommandBindComputeSets() = delete;
    RCommandBindComputeSets(uint32_t firstSet, uint32_t setCount, RSet* sets);
};

/// @brief Capture of RCOMMAND_BIND_VERTEX_BUFFERS
struct RCommandBindVertexBuffers
{
    const RCommandType type = RCOMMAND_BIND_VERTEX_BUFFERS;
    uint32_t firstBinding;
    std::vector<RBuffer> buffers;

    RCommandBindVertexBuffers() = delete;
    RCommandBindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, RBuffer* buffers);
};

/// @brief Capture of RCOMMAND_BIND_INDEX_BUFFER
struct RCommandBindIndexBuffer
{
    const RCommandType type = RCOMMAND_BIND_INDEX_BUFFER;
    RBuffer buffer;
    RIndexType indexType;

    RCommandBindIndexBuffer() = delete;
    RCommandBindIndexBuffer(RBuffer buffer, RIndexType indexType);
};

/// @brief Capture of RCOMMAND_DISPATCH
struct RCommandDispatch
{
    const RCommandType type = RCOMMAND_DISPATCH;
    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;

    RCommandDispatch() = delete;
    RCommandDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
};

/// @brief Capture of RCOMMAND_SET_SCISSOR
struct RCommandSetScissor
{
    const RCommandType type = RCOMMAND_SET_SCISSOR;
    Rect scissor;

    RCommandSetScissor() = delete;
    RCommandSetScissor(const Rect& scissor);
};

/// @brief Capture of RCOMMAND_DRAW
struct RCommandDraw
{
    const RCommandType type = RCOMMAND_DRAW;
    RDrawInfo drawInfo;

    RCommandDraw() = delete;
    RCommandDraw(const RDrawInfo& info);
};

/// @brief Capture of RCOMMAND_DRAW_INDEXED
struct RCommandDrawIndexed
{
    const RCommandType type = RCOMMAND_DRAW_INDEXED;
    RDrawIndexedInfo drawIndexedInfo;

    RCommandDrawIndexed() = delete;
    RCommandDrawIndexed(const RDrawIndexedInfo& info);
};

/// @brief Capture of RCOMMAND_DRAW_INDIRECT
struct RCommandDrawIndirect
{
    const RCommandType type = RCOMMAND_DRAW_INDIRECT;
    RDrawIndirectInfo drawIndirectInfo;

    RCommandDrawIndirect() = delete;
    RCommandDrawIndirect(const RDrawIndirectInfo& info);
};

/// @brief Capture of RCOMMAND_DRAW_INDEXED_INDIRECT
struct RCommandDrawIndexedIndirect
{
    const RCommandType type = RCOMMAND_DRAW_INDEXED_INDIRECT;
    RDrawIndexedIndirectInfo drawIndexedIndirectInfo;

    RCommandDrawIndexedIndirect() = delete;
    RCommandDrawIndexedIndirect(const RDrawIndexedIndirectInfo& info);
};

/// @brief Capture of RCOMMAND_BUFFER_MEMORY_BARRIER
struct RCommandBufferMemoryBarrier
{
    const RCommandType type = RCOMMAND_BUFFER_MEMORY_BARRIER;
    RPipelineStageFlags srcStages;
    RPipelineStageFlags dstStages;
    RBufferMemoryBarrier barrier;

    RCommandBufferMemoryBarrier() = delete;
    RCommandBufferMemoryBarrier(RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RBufferMemoryBarrier& barrier);
};

struct RCommandImageMemoryBarrier
{
    const RCommandType type = RCOMMAND_IMAGE_MEMORY_BARRIER;
    RPipelineStageFlags srcStages;
    RPipelineStageFlags dstStages;
    RImageMemoryBarrier barrier;

    RCommandImageMemoryBarrier() = delete;
    RCommandImageMemoryBarrier(RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RImageMemoryBarrier& barrier);
};

/// @brief Capture of RCOMMAND_COPY_BUFFER
struct RCommandCopyBuffer
{
    const RCommandType type = RCOMMAND_COPY_BUFFER;
    RBuffer srcBuffer;
    RBuffer dstBuffer;
    std::vector<RBufferCopy> regions;

    RCommandCopyBuffer() = delete;
    RCommandCopyBuffer(RBuffer srcBuffer, RBuffer dstBuffer, uint32_t regionCount, const RBufferCopy* regions);
};

/// @brief Capture of RCOMMAND_COPY_BUFFER_TO_IMAGE
struct RCommandCopyBufferToImage
{
    const RCommandType type = RCOMMAND_COPY_BUFFER_TO_IMAGE;
    RBuffer srcBuffer;
    RImage dstImage;
    RImageLayout dstImageLayout;
    std::vector<RBufferImageCopy> regions;

    RCommandCopyBufferToImage() = delete;
    RCommandCopyBufferToImage(RBuffer srcBuffer, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RBufferImageCopy* regions);
};

/// @brief Capture of RCOMMAND_COPY_IMAGE_TO_BUFFER
struct RCommandCopyImageToBuffer
{
    const RCommandType type = RCOMMAND_COPY_IMAGE_TO_BUFFER;
    RBuffer dstBuffer;
    RImage srcImage;
    RImageLayout srcImageLayout;
    std::vector<RBufferImageCopy> regions;

    RCommandCopyImageToBuffer() = delete;
    RCommandCopyImageToBuffer(RImage srcImage, RImageLayout srcImageLayout, RBuffer dstBuffer, uint32_t regionCount, const RBufferImageCopy* regions);
};

void render_command_placement_delete(const RCommandType* type);

} // namespace LD
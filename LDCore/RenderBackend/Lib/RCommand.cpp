#include <algorithm>

#include "RCommand.h"
#include "RUtilCommon.h"

namespace LD {

RCommandBeginPass::RCommandBeginPass(const RPassBeginInfo& passBI, RFramebufferObj* framebufferObj)
    : width(passBI.width), height(passBI.height), framebufferObj(framebufferObj)
{
    const uint32_t colorAttachmentCount = passBI.colorAttachmentCount;

    depthStencilAttachment = passBI.depthStencilAttachment;

    if (passBI.colorAttachments)
    {
        colorAttachments.resize(colorAttachmentCount);
        std::copy(passBI.colorAttachments, passBI.colorAttachments + colorAttachmentCount, colorAttachments.begin());
    }

    if (passBI.colorResolveAttachments)
    {
        colorResolveAttachments.resize(colorAttachmentCount);
        std::copy(passBI.colorResolveAttachments, passBI.colorResolveAttachments + colorAttachmentCount, colorResolveAttachments.begin());
    }

    if (passBI.clearColors)
    {
        clearColors.resize(colorAttachmentCount);
        std::copy(passBI.clearColors, passBI.clearColors + colorAttachmentCount, clearColors.begin());
    }

    clearDepthStencil = passBI.clearDepthStencil;
    RUtil::save_pass_info(passBI.pass, pass);
}

RCommandPushConstant::RCommandPushConstant(uint32_t offset, uint32_t size, const void* data)
    : offset(offset), size(size), data(data)
{
}

RCommandBindGraphicsPipeline::RCommandBindGraphicsPipeline(RPipeline pipeline)
    : pipeline(pipeline)
{
}

RCommandBindComputePipeline::RCommandBindComputePipeline(RPipeline pipeline)
    : pipeline(pipeline)
{
}

RCommandBindGraphicsSets::RCommandBindGraphicsSets(uint32_t firstSet, uint32_t setCount, RSet* pSets)
    : firstSet(firstSet)
{
    sets.resize(setCount);
    std::copy(pSets, pSets + setCount, sets.begin());
}

RCommandBindComputeSets::RCommandBindComputeSets(uint32_t firstSet, uint32_t setCount, RSet* pSets)
    : firstSet(firstSet)
{
    sets.resize(setCount);
    std::copy(pSets, pSets + setCount, sets.begin());
}

RCommandBindVertexBuffers::RCommandBindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, RBuffer* vertexBuffers)
    : firstBinding(firstBinding)
{
    buffers.resize(bindingCount);
    std::copy(vertexBuffers, vertexBuffers + bindingCount, buffers.begin());
}

RCommandBindIndexBuffer::RCommandBindIndexBuffer(RBuffer buffer, RIndexType indexType)
    : buffer(buffer), indexType(indexType)
{
}

RCommandDispatch::RCommandDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    : groupCountX(groupCountX), groupCountY(groupCountY), groupCountZ(groupCountZ)
{
}

RCommandSetScissor::RCommandSetScissor(const Rect& scissor)
    : scissor(scissor)
{
}

RCommandDraw::RCommandDraw(const RDrawInfo& info)
    : drawInfo(info)
{
}

RCommandDrawIndexed::RCommandDrawIndexed(const RDrawIndexedInfo& info)
    : drawIndexedInfo(info)
{
}

RCommandDrawIndirect::RCommandDrawIndirect(const RDrawIndirectInfo& info)
    : drawIndirectInfo(info)
{
}

RCommandBufferMemoryBarrier::RCommandBufferMemoryBarrier(RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RBufferMemoryBarrier& barrier)
    : srcStages(srcStages), dstStages(dstStages), barrier(barrier)
{
}

RCommandImageMemoryBarrier::RCommandImageMemoryBarrier(RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RImageMemoryBarrier& barrier)
    : srcStages(srcStages), dstStages(dstStages), barrier(barrier)
{
}

RCommandCopyBuffer::RCommandCopyBuffer(RBuffer srcBuffer, RBuffer dstBuffer, uint32_t regionCount, const RBufferCopy* pRegions)
    : srcBuffer(srcBuffer), dstBuffer(dstBuffer)
{
    regions.resize(regionCount);
    std::copy(pRegions, pRegions + regionCount, regions.begin());
}

RCommandCopyBufferToImage::RCommandCopyBufferToImage(RBuffer srcBuffer, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RBufferImageCopy* pRegions)
    : srcBuffer(srcBuffer), dstImage(dstImage), dstImageLayout(dstImageLayout)
{
    regions.resize(regionCount);
    std::copy(pRegions, pRegions + regionCount, regions.begin());
}

RCommandCopyImageToBuffer::RCommandCopyImageToBuffer(RImage srcImage, RImageLayout srcImageLayout, RBuffer dstBuffer, uint32_t regionCount, const RBufferImageCopy* copyRegions)
    : srcImage(srcImage), srcImageLayout(srcImageLayout), dstBuffer(dstBuffer)
{
    regions.resize(regionCount);
    std::copy(copyRegions, copyRegions + regionCount, regions.begin());
}

void render_command_placement_delete(const RCommandType* type)
{
    switch (*type)
    {
    case RCOMMAND_BEGIN_PASS:
        ((RCommandBeginPass*)(type))->~RCommandBeginPass();
        break;
    case RCOMMAND_PUSH_CONSTANT:
        ((RCommandPushConstant*)(type))->~RCommandPushConstant();
        break;
    case RCOMMAND_BIND_GRAPHICS_PIPELINE:
        ((RCommandBindGraphicsPipeline*)(type))->~RCommandBindGraphicsPipeline();
        break;
    case RCOMMAND_BIND_GRAPHICS_SETS:
        ((RCommandBindGraphicsSets*)(type))->~RCommandBindGraphicsSets();
        break;
    case RCOMMAND_BIND_COMPUTE_PIPELINE:
        ((RCommandBindComputePipeline*)(type))->~RCommandBindComputePipeline();
        break;
    case RCOMMAND_BIND_COMPUTE_SETS:
        ((RCommandBindComputeSets*)(type))->~RCommandBindComputeSets();
        break;
    case RCOMMAND_BIND_VERTEX_BUFFERS:
        ((RCommandBindVertexBuffers*)(type))->~RCommandBindVertexBuffers();
        break;
    case RCOMMAND_BIND_INDEX_BUFFER:
        ((RCommandBindIndexBuffer*)(type))->~RCommandBindIndexBuffer();
        break;
    case RCOMMAND_SET_SCISSOR:
        ((RCommandSetScissor*)(type))->~RCommandSetScissor();
        break;
    case RCOMMAND_DRAW:
        ((RCommandDraw*)(type))->~RCommandDraw();
        break;
    case RCOMMAND_DRAW_INDEXED:
        ((RCommandDrawIndexed*)(type))->~RCommandDrawIndexed();
        break;
    case RCOMMAND_DRAW_INDIRECT:
        ((RCommandDrawIndirect*)(type))->~RCommandDrawIndirect();
        break;
    case RCOMMAND_DISPATCH:
        ((RCommandDispatch*)(type))->~RCommandDispatch();
        break;
    case RCOMMAND_BUFFER_MEMORY_BARRIER:
        ((RCommandBufferMemoryBarrier*)(type))->~RCommandBufferMemoryBarrier();
        break;
    case RCOMMAND_IMAGE_MEMORY_BARRIER:
        ((RCommandImageMemoryBarrier*)(type))->~RCommandImageMemoryBarrier();
        break;
    case RCOMMAND_COPY_BUFFER:
        ((RCommandCopyBuffer*)(type))->~RCommandCopyBuffer();
        break;
    case RCOMMAND_COPY_BUFFER_TO_IMAGE:
        ((RCommandCopyBufferToImage*)(type))->~RCommandCopyBufferToImage();
        break;
    case RCOMMAND_COPY_IMAGE_TO_BUFFER:
        ((RCommandCopyImageToBuffer*)(type))->~RCommandCopyImageToBuffer();
        break;
    case RCOMMAND_END_PASS:
    case RCOMMAND_BLIT_IMAGE:
    default:
        break;
    }
}

} // namespace LD
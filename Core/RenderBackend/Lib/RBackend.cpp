#include "RBackendObj.h"
#include "RUtilInternal.h"
#include <Ludens/DSA/Hash.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/System/Memory.h>

namespace LD {

uint64_t RObjectID::sCounter = 0;

void RQueue::wait_idle()
{
    mObj->wait_idle(mObj);
}

void RQueue::submit(const RSubmitInfo& submitI, RFence fence)
{
    mObj->submit(mObj, submitI, fence);
}

RDevice RDevice::create(const RDeviceInfo& info)
{
    RDeviceObj* obj = (RDeviceObj*)heap_malloc(sizeof(RDeviceObj), MEMORY_USAGE_RENDER);
    obj->rid = RObjectID::get();

    if (info.backend == RDEVICE_BACKEND_VULKAN)
        vk_create_device(obj, info);
    else
        LD_UNREACHABLE;

    return {obj};
}

void RDevice::destroy(RDevice device)
{
    if (device.mObj->backend == RDEVICE_BACKEND_VULKAN)
        vk_destroy_device(device.mObj);
    else
        LD_UNREACHABLE;

    heap_free(device.mObj);
}

RSemaphore RDevice::create_semaphore()
{
    RSemaphoreObj* semaphoreObj = (RSemaphoreObj*)heap_malloc(sizeof(RSemaphoreObj), MEMORY_USAGE_RENDER);
    semaphoreObj->rid = RObjectID::get();

    return mObj->create_semaphore(mObj, semaphoreObj);
}

void RDevice::destroy_semaphore(RSemaphore semaphore)
{
    mObj->destroy_semaphore(mObj, semaphore);

    heap_free((RSemaphoreObj*)semaphore);
}

RFence RDevice::create_fence(bool createSignaled)
{
    RFenceObj* fenceObj = (RFenceObj*)heap_malloc(sizeof(RFenceObj), MEMORY_USAGE_RENDER);
    fenceObj->rid = RObjectID::get();

    return mObj->create_fence(mObj, createSignaled, fenceObj);
}

void RDevice::destroy_fence(RFence fence)
{
    mObj->destroy_fence(mObj, fence);

    heap_free((RFenceObj*)fence);
}

RBuffer RDevice::create_buffer(const RBufferInfo& bufferI)
{
    RBufferObj* bufferObj = (RBufferObj*)heap_malloc(sizeof(RBufferObj), MEMORY_USAGE_RENDER);
    bufferObj->rid = RObjectID::get();
    bufferObj->info = bufferI;
    bufferObj->device = *this;
    bufferObj->hostMap = nullptr;

    return mObj->create_buffer(mObj, bufferI, bufferObj);
}

void RDevice::destroy_buffer(RBuffer buffer)
{
    mObj->destroy_buffer(mObj, buffer);

    heap_free((RBufferObj*)buffer);
}

RImage RDevice::create_image(const RImageInfo& imageI)
{
    RImageObj* imageObj = (RImageObj*)heap_malloc(sizeof(RImageObj), MEMORY_USAGE_RENDER);
    imageObj->rid = RObjectID::get();
    imageObj->info = imageI;

    return mObj->create_image(mObj, imageI, imageObj);
}

void RDevice::destroy_image(RImage image)
{
    mObj->destroy_image(mObj, image);

    heap_free((RImageObj*)image);
}

RPass RDevice::create_pass(const RPassInfo& passI)
{
    RPassObj* passObj = (RPassObj*)heap_malloc(sizeof(RPassObj), MEMORY_USAGE_RENDER);
    passObj->rid = RObjectID::get();
    passObj->hash = hash32_pass_info(passI);
    passObj->colorAttachmentCount = passI.colorAttachmentCount;
    passObj->hasDepthStencilAttachment = passI.depthStencilAttachment != nullptr;

    return mObj->create_pass(mObj, passI, passObj);
}

void RDevice::destroy_pass(RPass pass)
{
    mObj->destroy_pass(mObj, pass);

    heap_free((RPassObj*)pass);
}

RFramebuffer RDevice::create_framebuffer(const RFramebufferInfo& fbI)
{
    RFramebufferObj* framebufferObj = (RFramebufferObj*)heap_malloc(sizeof(RFramebufferObj), MEMORY_USAGE_RENDER);
    framebufferObj->rid = RObjectID::get();
    framebufferObj->width = fbI.width;
    framebufferObj->height = fbI.height;
    framebufferObj->pass = fbI.pass;

    return mObj->create_framebuffer(mObj, fbI, framebufferObj);
}

void RDevice::destroy_framebuffer(RFramebuffer fb)
{
    mObj->destroy_framebuffer(mObj, fb);

    heap_free((RFramebufferObj*)fb);
}

RCommandPool RDevice::create_command_pool(const RCommandPoolInfo& poolI)
{
    RCommandPoolObj* poolObj = (RCommandPoolObj*)heap_malloc(sizeof(RCommandPoolObj), MEMORY_USAGE_RENDER);
    poolObj->rid = RObjectID::get();

    return mObj->create_command_pool(mObj, poolI, poolObj);
}

void RDevice::destroy_command_pool(RCommandPool pool)
{
    RCommandPoolObj* poolObj = (RCommandPoolObj*)pool;
    LD_ASSERT(poolObj->commandBufferCount == 0);

    mObj->destroy_command_pool(mObj, pool);

    heap_free((RCommandPoolObj*)pool);
}

RShader RDevice::create_shader(const RShaderInfo& shaderI)
{
    RShaderObj* shaderObj = (RShaderObj*)heap_malloc(sizeof(RShaderObj), MEMORY_USAGE_RENDER);
    shaderObj->rid = RObjectID::get();
    shaderObj->type = shaderI.type;

    return mObj->create_shader(mObj, shaderI, shaderObj);
}

void RDevice::destroy_shader(RShader shader)
{
    mObj->destroy_shader(mObj, shader);

    heap_free((RShaderObj*)shader);
}

RSetPool RDevice::create_set_pool(const RSetPoolInfo& poolI)
{
    RSetPoolObj* poolObj = (RSetPoolObj*)heap_malloc(sizeof(RSetPoolObj), MEMORY_USAGE_RENDER);
    poolObj->rid = RObjectID::get();
    new (&poolObj->setLA) LinearAllocator();
    poolObj->setLA.create(sizeof(RSetObj) * poolI.maxSets, MEMORY_USAGE_RENDER);

    return mObj->create_set_pool(mObj, poolI, poolObj);
}

void RDevice::destroy_set_pool(RSetPool pool)
{
    RSetPoolObj* poolObj = (RSetPoolObj*)pool;

    mObj->destroy_set_pool(mObj, pool);

    poolObj->setLA.destroy();
    poolObj->setLA.~LinearAllocator();
    heap_free((RSetPoolObj*)pool);
}

RSetLayout RDevice::create_set_layout(const RSetLayoutInfo& layoutI)
{
    RSetLayoutObj* layoutObj = (RSetLayoutObj*)heap_malloc(sizeof(RSetLayoutObj), MEMORY_USAGE_RENDER);
    layoutObj->rid;
    layoutObj->hash = hash32_set_layout_info(layoutI);

    return mObj->create_set_layout(mObj, layoutI, layoutObj);
}

void RDevice::destroy_set_layout(RSetLayout layout)
{
    mObj->destroy_set_layout(mObj, layout);

    heap_free((RSetLayoutObj*)layout);
}

RPipelineLayout RDevice::create_pipeline_layout(const RPipelineLayoutInfo& layoutI)
{
    LD_ASSERT(layoutI.setLayoutCount <= PIPELINE_LAYOUT_MAX_RESOURCE_SETS);

    RPipelineLayoutObj* layoutObj = (RPipelineLayoutObj*)heap_malloc(sizeof(RPipelineLayoutObj), MEMORY_USAGE_RENDER);
    layoutObj->rid = RObjectID::get();
    layoutObj->hash = hash32_pipeline_layout_info(layoutI);
    layoutObj->set_count = layoutI.setLayoutCount;

    for (uint32_t i = 0; i < layoutObj->set_count; i++)
        layoutObj->set_layouts[i] = layoutI.setLayouts[i];

    return mObj->create_pipeline_layout(mObj, layoutI, layoutObj);
}

void RDevice::destroy_pipeline_layout(RPipelineLayout layout)
{
    mObj->destroy_pipeline_layout(mObj, layout);

    heap_free((RPipelineLayoutObj*)layout);
}

RPipeline RDevice::create_pipeline(const RPipelineInfo& pipelineI)
{
    RPipelineObj* pipelineObj = (RPipelineObj*)heap_malloc(sizeof(RPipelineObj), MEMORY_USAGE_RENDER);
    pipelineObj->rid = RObjectID::get();
    pipelineObj->layout = pipelineI.layout;

    return mObj->create_pipeline(mObj, pipelineI, pipelineObj);
}

void RDevice::destroy_pipeline(RPipeline pipeline)
{
    mObj->destroy_pipeline(mObj, pipeline);

    heap_free((RPipelineObj*)pipeline);
}

void RDevice::update_set_images(uint32_t updateCount, const RSetImageUpdateInfo* updates)
{
    mObj->update_set_images(mObj, updateCount, updates);
}

void RDevice::update_set_buffers(uint32_t updateCount, const RSetBufferUpdateInfo* updates)
{
    mObj->update_set_buffers(mObj, updateCount, updates);
}

uint32_t RDevice::next_frame(RSemaphore& imageAcquired, RSemaphore& presentReady, RFence& frameComplete)
{
    return mObj->next_frame(mObj, imageAcquired, presentReady, frameComplete);
}

void RDevice::present_frame()
{
    return mObj->present_frame(mObj);
}

RImage RDevice::get_swapchain_color_attachment(uint32_t frameIdx)
{
    return mObj->get_swapchain_color_attachment(mObj, frameIdx);
}

uint32_t RDevice::get_swapchain_image_count()
{
    return mObj->get_swapchain_image_count(mObj);
}

uint32_t RDevice::get_frames_in_flight_count()
{
    return mObj->get_frames_in_flight_count(mObj);
}

RQueue RDevice::get_graphics_queue()
{
    return mObj->get_graphics_queue(mObj);
}

void RDevice::wait_idle()
{
    return mObj->wait_idle(mObj);
}

RImageUsageFlags RImage::usage() const
{
    return mObj->info.usage;
}

RImageType RImage::type() const
{
    return mObj->info.type;
}

RFormat RImage::format() const
{
    return mObj->info.format;
}

uint32_t RImage::width() const
{
    return mObj->info.width;
}

uint32_t RImage::height() const
{
    return mObj->info.height;
}

uint32_t RImage::depth() const
{
    return mObj->info.depth;
}

uint64_t RImage::size() const
{
    uint32_t texelSize = RUtil::get_format_texel_size(mObj->info.format);
    uint64_t layerSize = mObj->info.width * mObj->info.height * mObj->info.depth;

    return layerSize * texelSize;
}

uint64_t RBuffer::size() const
{
    return mObj->info.size;
}

RBufferUsageFlags RBuffer::usage() const
{
    return mObj->info.usage;
}

void RBuffer::map()
{
    LD_ASSERT(mObj->info.hostVisible);
    LD_ASSERT(mObj->hostMap == nullptr);

    mObj->map(mObj);
}

void RBuffer::map_write(uint64_t offset, uint64_t size, const void* data)
{
    LD_ASSERT(mObj->hostMap != nullptr);
    LD_ASSERT(offset + size <= mObj->info.size);

    mObj->map_write(mObj, offset, size, data);
}

void RBuffer::unmap()
{
    LD_ASSERT(mObj->hostMap != nullptr);

    mObj->unmap(mObj);

    mObj->hostMap = nullptr;
}

uint32_t RFramebuffer::width() const
{
    return mObj->width;
}

uint32_t RFramebuffer::height() const
{
    return mObj->height;
}

RPass RFramebuffer::pass() const
{
    return mObj->pass;
}

void RCommandList::free()
{
    mObj->poolObj->commandBufferCount--;

    mObj->free(mObj);

    // nulify the handle
    mObj = nullptr;
}

void RCommandList::begin()
{
    mObj->begin(mObj, false);
}

void RCommandList::end()
{
    mObj->end(mObj);
}

void RCommandList::cmd_begin_pass(const RPassBeginInfo& passBI)
{
    mObj->cmd_begin_pass(mObj, passBI);
}

void RCommandList::cmd_bind_graphics_pipeline(RPipeline pipeline)
{
    mObj->cmd_bind_graphics_pipeline(mObj, pipeline);
}

void RCommandList::cmd_bind_graphics_sets(RPipelineLayout layout, uint32_t firstSet, uint32_t setCount, RSet* sets)
{
    mObj->cmd_bind_graphics_sets(mObj, layout, firstSet, setCount, sets);
}

void RCommandList::cmd_bind_vertex_buffers(uint32_t firstBinding, uint32_t bindingCount, RBuffer* buffers)
{
    for (uint32_t i = 0; i < bindingCount; i++)
        LD_ASSERT(buffers[i].usage() & RBUFFER_USAGE_VERTEX_BIT);

    mObj->cmd_bind_vertex_buffers(mObj, firstBinding, bindingCount, buffers);
}

void RCommandList::cmd_bind_index_buffer(RBuffer buffer, RIndexType indexType)
{
    LD_ASSERT(buffer.usage() & RBUFFER_USAGE_INDEX_BIT);

    mObj->cmd_bind_index_buffer(mObj, buffer, indexType);
}

void RCommandList::cmd_draw(const RDrawInfo& drawI)
{
    mObj->cmd_draw(mObj, drawI);
}

void RCommandList::cmd_draw_indices(const RDrawIndexedInfo& drawI)
{
    mObj->cmd_draw_indexed(mObj, drawI);
}

void RCommandList::cmd_end_pass()
{
    mObj->cmd_end_pass(mObj);
}

void RCommandList::cmd_image_memory_barrier(RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RImageMemoryBarrier& barrier)
{
    mObj->cmd_image_memory_barrier(mObj, srcStages, dstStages, barrier);
}

void RCommandList::cmd_copy_buffer(RBuffer srcBuffer, RBuffer dstBuffer, uint32_t regionCount, const RBufferCopy* regions)
{
    LD_ASSERT(srcBuffer.usage() & RBUFFER_USAGE_TRANSFER_SRC_BIT);
    LD_ASSERT(dstBuffer.usage() & RBUFFER_USAGE_TRANSFER_DST_BIT);

    mObj->cmd_copy_buffer(mObj, srcBuffer, dstBuffer, regionCount, regions);
}

void RCommandList::cmd_copy_buffer_to_image(RBuffer srcBuffer, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RBufferImageCopy* regions)
{
    LD_ASSERT(srcBuffer.usage() & RBUFFER_USAGE_TRANSFER_SRC_BIT);
    LD_ASSERT(dstImage.usage() & RIMAGE_USAGE_TRANSFER_DST_BIT);

    mObj->cmd_copy_buffer_to_image(mObj, srcBuffer, dstImage, dstImageLayout, regionCount, regions);
}

void RCommandList::cmd_blit_image(RImage srcImage, RImageLayout srcImageLayout, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RImageBlit* regions, RFilter filter)
{
    mObj->cmd_blit_image(mObj, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, regions, filter);
}

RCommandList RCommandPool::allocate()
{
    // NOTE: command pools (and its allocated command buffers) are never
    //       shared among threads, so this counter doesn't have to be atomic
    mObj->commandBufferCount++;

    return mObj->allocate(mObj);
}

uint32_t hash32_pass_info(const RPassInfo& passI)
{
    std::string str = std::to_string(passI.colorAttachmentCount);

    for (uint32_t i = 0; i < passI.colorAttachmentCount; i++)
    {
        const RPassColorAttachment* attachment = passI.colorAttachments + i;
        str.push_back('c');
        str += std::to_string((int)attachment->colorFormat);
        str.push_back('l');
        str += std::to_string((int)attachment->colorLoadOp);
        str.push_back('s');
        str += std::to_string((int)attachment->colorStoreOp);
        str.push_back('i');
        str += std::to_string((int)attachment->initialLayout);
        str.push_back('p');
        str += std::to_string((int)attachment->passLayout);
        str.push_back('f');
        str += std::to_string((int)attachment->finalLayout);
    }

    if (passI.depthStencilAttachment)
    {
        const RPassDepthStencilAttachment* attachment = passI.depthStencilAttachment;
        str.push_back('d');
        str += std::to_string((int)attachment->depthStencilFormat);
        str.push_back('l');
        str += std::to_string((int)attachment->depthLoadOp);
        str.push_back('s');
        str += std::to_string((int)attachment->depthStoreOp);
        str.push_back('l');
        str += std::to_string((int)attachment->stencilLoadOp);
        str.push_back('s');
        str += std::to_string((int)attachment->stencilStoreOp);
        str.push_back('i');
        str += std::to_string((int)attachment->initialLayout);
        str.push_back('p');
        str += std::to_string((int)attachment->passLayout);
        str.push_back('f');
        str += std::to_string((int)attachment->finalLayout);
    }

    if (passI.srcDependency)
    {
        const RPassDependency* dep = passI.srcDependency;
        str.push_back('S');
        str += std::to_string(dep->srcStageMask);
        str.push_back('_');
        str += std::to_string(dep->dstStageMask);
        str.push_back('_');
        str += std::to_string(dep->srcAccessMask);
        str.push_back('_');
        str += std::to_string(dep->dstAccessMask);
    }

    if (passI.dstDependency)
    {
        const RPassDependency* dep = passI.dstDependency;
        str.push_back('D');
        str += std::to_string(dep->srcStageMask);
        str.push_back('_');
        str += std::to_string(dep->dstStageMask);
        str.push_back('_');
        str += std::to_string(dep->srcAccessMask);
        str.push_back('_');
        str += std::to_string(dep->dstAccessMask);
    }

    return hash32_FNV_1a(str.data(), str.size());
}

uint32_t hash32_set_layout_info(const RSetLayoutInfo& layoutI)
{
    std::string str = std::to_string(layoutI.bindingCount);

    for (uint32_t i = 0; i < layoutI.bindingCount; i++)
    {
        str.push_back('b');
        str += std::to_string(layoutI.bindings[i].binding);
        str.push_back('t');
        str += std::to_string((int)layoutI.bindings[i].type);
        str.push_back('a');
        str += std::to_string(layoutI.bindings[i].arrayCount);
    }

    return hash32_FNV_1a(str.data(), str.size());
}

uint32_t hash32_pipeline_layout_info(const RPipelineLayoutInfo& layoutI)
{
    if (layoutI.setLayoutCount == 0)
        return 0;

    // NOTE: if a pipeline layout only has a single set layout,
    //       the pipeline layout hash will be equivalent to
    //       the set layout hash, but this shouldn't be an issue.
    std::size_t hash = layoutI.setLayouts[0].hash();

    for (uint32_t i = 1; i < layoutI.setLayoutCount; i++)
        hash_combine(hash, layoutI.setLayouts[i].hash());

    // TODO: this truncates since std::size_t is most likely 8 bytes on 64-bit systems
    return (uint32_t)hash;
}

uint32_t hash32_pipeline_rasterization_state(const RPipelineRasterizationInfo& rasterizationI)
{
    std::string str;

    str.push_back('c');
    str += std::to_string((int)rasterizationI.cullMode);
    str.push_back('p');
    str += std::to_string((int)rasterizationI.polygonMode);

    if (rasterizationI.polygonMode == RPOLYGON_MODE_LINE)
    {
        str.push_back('l');
        str += std::to_string(rasterizationI.lineWidth);
    }

    return hash32_FNV_1a(str.data(), str.size());
}

uint32_t RPass::hash() const
{
    return mObj->hash;
}

uint32_t RPass::color_attachment_count() const
{
    return mObj->colorAttachmentCount;
}

bool RPass::has_depth_stencil_attachment() const
{
    return mObj->hasDepthStencilAttachment;
}

uint32_t RSetLayout::hash() const
{
    return mObj->hash;
}

RSet RSetPool::allocate(RSetLayout layout)
{
    RSetObj* setObj = (RSetObj*)mObj->setLA.allocate(sizeof(RSetObj));

    return mObj->allocate(mObj, layout, setObj);
}

void RSetPool::reset()
{
    mObj->setLA.free();

    return mObj->reset(mObj);
}

uint32_t RPipelineLayout::hash() const
{
    return mObj->hash;
}

uint32_t RPipelineLayout::resource_set_count() const
{
    return mObj->set_count;
}

RSetLayout RPipelineLayout::resource_set_layout(int32_t index) const
{
    if (index >= mObj->set_count)
        return {};

    return mObj->set_layouts[index];
}

RPipelineLayout RPipeline::layout() const
{
    return mObj->layout;
}

} // namespace LD
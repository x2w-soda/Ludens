#include "RBackendObj.h"
#include "RUtil.h"
#include <Ludens/DSA/Hash.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/System/Memory.h>

namespace LD {

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
    return mObj->create_semaphore(mObj);
}

void RDevice::destroy_semaphore(RSemaphore semaphore)
{
    mObj->destroy_semaphore(mObj, semaphore);
}

RFence RDevice::create_fence(bool createSignaled)
{
    return mObj->create_fence(mObj, createSignaled);
}

void RDevice::destroy_fence(RFence fence)
{
    mObj->destroy_fence(mObj, fence);
}

RBuffer RDevice::create_buffer(const RBufferInfo& bufferI)
{
    RBuffer buffer = mObj->create_buffer(mObj, bufferI);

    RBufferObj* bufferObj = static_cast<RBufferObj*>(buffer);
    bufferObj->info = bufferI;
    bufferObj->device = *this;
    bufferObj->hostMap = nullptr;

    return buffer;
}

void RDevice::destroy_buffer(RBuffer buffer)
{
    mObj->destroy_buffer(mObj, buffer);
}

RImage RDevice::create_image(const RImageInfo& imageI)
{
    RImage image = mObj->create_image(mObj, imageI);

    RImageObj* imageObj = static_cast<RImageObj*>(image);
    imageObj->info = imageI;

    return image;
}

void RDevice::destroy_image(RImage image)
{
    mObj->destroy_image(mObj, image);
}

RPass RDevice::create_pass(const RPassInfo& passI)
{
    RPass pass = mObj->create_pass(mObj, passI);

    static_cast<RPassObj*>(pass)->hash = hash32_pass_info(passI);

    return pass;
}

void RDevice::destroy_pass(RPass pass)
{
    mObj->destroy_pass(mObj, pass);
}

RFramebuffer RDevice::create_framebuffer(const RFramebufferInfo& fbI)
{
    return mObj->create_framebuffer(mObj, fbI);
}

void RDevice::destroy_framebuffer(RFramebuffer fb)
{
    mObj->destroy_framebuffer(mObj, fb);
}

RCommandPool RDevice::create_command_pool(const RCommandPoolInfo& poolI)
{
    return mObj->create_command_pool(mObj, poolI);
}

void RDevice::destroy_command_pool(RCommandPool pool)
{
    RCommandPoolObj* poolObj = (RCommandPoolObj*)pool;
    LD_ASSERT(poolObj->commandBufferCount == 0);

    mObj->destroy_command_pool(mObj, pool);
}

RShader RDevice::create_shader(const RShaderInfo& shaderI)
{
    RShader shader = mObj->create_shader(mObj, shaderI);

    RShaderObj* shaderObj = (RShaderObj*)shader;
    shaderObj->type = shaderI.type;

    return shader;
}

void RDevice::destroy_shader(RShader shader)
{
    mObj->destroy_shader(mObj, shader);
}

RSetPool RDevice::create_set_pool(const RSetPoolInfo& poolI)
{
    return mObj->create_set_pool(mObj, poolI);
}

void RDevice::destroy_set_pool(RSetPool pool)
{
    mObj->destroy_set_pool(mObj, pool);
}

RSetLayout RDevice::create_set_layout(const RSetLayoutInfo& layoutI)
{
    RSetLayout layout = mObj->create_set_layout(mObj, layoutI);

    static_cast<RSetLayoutObj*>(layout)->hash = hash32_set_layout_info(layoutI);

    return layout;
}

void RDevice::destroy_set_layout(RSetLayout layout)
{
    mObj->destroy_set_layout(mObj, layout);
}

RPipelineLayout RDevice::create_pipeline_layout(const RPipelineLayoutInfo& layoutI)
{
    LD_ASSERT(layoutI.setLayoutCount <= PIPELINE_LAYOUT_MAX_RESOURCE_SETS);

    RPipelineLayout layout = mObj->create_pipeline_layout(mObj, layoutI);

    static_cast<RPipelineLayoutObj*>(layout)->hash = hash32_pipeline_layout_info(layoutI);

    return layout;
}

void RDevice::destroy_pipeline_layout(RPipelineLayout layout)
{
    mObj->destroy_pipeline_layout(mObj, layout);
}

RPipeline RDevice::create_pipeline(const RPipelineInfo& pipelineI)
{
    return mObj->create_pipeline(mObj, pipelineI);
}

void RDevice::destroy_pipeline(RPipeline pipeline)
{
    mObj->destroy_pipeline(mObj, pipeline);
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

RQueue RDevice::get_graphics_queue()
{
    return mObj->get_graphics_queue(mObj);
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
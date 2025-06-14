#include "RBackendObj.h"
#include "RUtilInternal.h"
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/System/Memory.h>
#include <unordered_map>

namespace LD {

static Log sLog("RBackend");
std::unordered_map<uint32_t, RPassObj*> sPasses;
std::unordered_map<uint32_t, RSetLayoutObj*> sSetLayouts;
std::unordered_map<uint32_t, RPipelineLayoutObj*> sPipelineLayouts;
std::unordered_map<uint32_t, RFramebufferObj*> sFramebuffers;
uint64_t RObjectID::sCounter = 0;

void RQueue::wait_idle()
{
    LD_PROFILE_SCOPE;

    mObj->wait_idle(mObj);
}

void RQueue::submit(const RSubmitInfo& submitI, RFence fence)
{
    LD_PROFILE_SCOPE;

    mObj->submit(mObj, submitI, fence);
}

RDevice RDevice::create(const RDeviceInfo& info)
{
    LD_PROFILE_SCOPE;

    RDeviceObj* obj = (RDeviceObj*)heap_malloc(sizeof(RDeviceObj), MEMORY_USAGE_RENDER);
    obj->rid = RObjectID::get();
    obj->frameIndex = 0;
    obj->isHeadless = info.window == nullptr;

    if (info.backend == RDEVICE_BACKEND_VULKAN)
        vk_create_device(obj, info);
    else
        LD_UNREACHABLE;

    return {obj};
}

void RDevice::destroy(RDevice device)
{
    LD_PROFILE_SCOPE;

    RDeviceObj* obj = device.mObj;

    for (auto& ite : sPipelineLayouts)
    {
        obj->destroy_pipeline_layout(obj, ite.second);
        heap_free(ite.second);
    }
    sLog.info("RDevice destroyed {} pipeline layouts", (int)sPipelineLayouts.size());
    sPipelineLayouts.clear();

    for (auto& ite : sSetLayouts)
    {
        obj->destroy_set_layout(obj, ite.second);
        heap_free(ite.second);
    }
    sLog.info("RDevice destroyed {} set layouts", (int)sSetLayouts.size());
    sSetLayouts.clear();

    for (auto& ite : sPasses)
    {
        obj->destroy_pass(obj, ite.second);
        heap_free(ite.second);
    }
    sLog.info("RDevice destroyed {} passes", (int)sPasses.size());
    sPasses.clear();

    // NOTE: destroying images also destroys all framebuffers that
    //       reference them, so we probably have zero framebuffers left already.
    for (auto& ite : sFramebuffers)
    {
        obj->destroy_framebuffer(obj, ite.second);
        heap_free(ite.second);
    }
    sLog.info("RDevice destroyed {} framebuffers", (int)sFramebuffers.size());
    sFramebuffers.clear();

    if (obj->backend == RDEVICE_BACKEND_VULKAN)
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
    // early sanity checks
    LD_ASSERT(!(imageI.type == RIMAGE_TYPE_2D && imageI.layers != 1));
    LD_ASSERT(!(imageI.type == RIMAGE_TYPE_CUBE && imageI.layers != 6));

    RImageObj* imageObj = (RImageObj*)heap_malloc(sizeof(RImageObj), MEMORY_USAGE_RENDER);
    mObj->create_image(mObj, imageI, imageObj);

    imageObj->rid = RObjectID::get();
    imageObj->info = imageI;
    imageObj->device = *this;

    return {imageObj};
}

void RDevice::destroy_image(RImage image)
{
    RImageObj* obj = image;

    if (obj->fboHashes.size() > 0)
    {
        // slow path, destroy all framebuffers using this image
        wait_idle();

        for (uint32_t fboHash : obj->fboHashes)
        {
            if (sFramebuffers.contains(fboHash))
            {
                mObj->destroy_framebuffer(mObj, sFramebuffers[fboHash]);
                heap_free((RFramebufferObj*)sFramebuffers[fboHash]);
                sFramebuffers.erase(fboHash);
            }
        }

        obj->fboHashes.clear();
    }

    mObj->destroy_image(mObj, image);
    heap_free(obj);
}

RCommandPool RDevice::create_command_pool(const RCommandPoolInfo& poolI)
{
    RCommandPoolObj* poolObj = heap_new<RCommandPoolObj>(MEMORY_USAGE_RENDER);
    poolObj->rid = RObjectID::get();
    poolObj->deviceObj = mObj;

    return mObj->create_command_pool(mObj, poolI, poolObj);
}

void RDevice::destroy_command_pool(RCommandPool pool)
{
    RCommandPoolObj* poolObj = (RCommandPoolObj*)pool;

    for (RCommandList list : poolObj->lists)
        heap_delete((RCommandListObj*)list);

    mObj->destroy_command_pool(mObj, pool);

    heap_delete(poolObj);
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
    poolObj->deviceObj = mObj;
    poolObj->layoutObj = mObj->get_or_create_set_layout_obj(poolI.layout);
    new (&poolObj->setLA) LinearAllocator();

    LinearAllocatorInfo laI{};
    laI.usage = MEMORY_USAGE_RENDER;
    laI.capacity = sizeof(RSetObj) * poolI.maxSets;
    poolObj->setLA = LinearAllocator::create(laI);

    return mObj->create_set_pool(mObj, poolI, poolObj);
}

void RDevice::destroy_set_pool(RSetPool pool)
{
    RSetPoolObj* poolObj = (RSetPoolObj*)pool;

    mObj->destroy_set_pool(mObj, pool);

    LinearAllocator::destroy(poolObj->setLA);

    poolObj->setLA.~LinearAllocator();
    heap_free((RSetPoolObj*)pool);
}

RPipeline RDevice::create_pipeline(const RPipelineInfo& pipelineI)
{
    RPipelineObj* pipelineObj = heap_new<RPipelineObj>(MEMORY_USAGE_RENDER);
    pipelineObj->init_vk_api();
    pipelineObj->rid = RObjectID::get();
    pipelineObj->variant.passObj = nullptr;
    pipelineObj->variant.depthTestEnabled = false;
    pipelineObj->deviceObj = mObj;
    pipelineObj->layoutObj = mObj->get_or_create_pipeline_layout_obj(pipelineI.layout);

    // NOTE: the exact render pass is only known during command recording,
    //       this only creates a shell object and the actual graphics API handle is not created yet.
    return mObj->create_pipeline(mObj, pipelineI, pipelineObj);
}

RPipeline RDevice::create_compute_pipeline(const RComputePipelineInfo& pipelineI)
{
    RPipelineObj* pipelineObj = heap_new<RPipelineObj>(MEMORY_USAGE_RENDER);
    pipelineObj->rid = RObjectID::get();
    pipelineObj->layoutObj = mObj->get_or_create_pipeline_layout_obj(pipelineI.layout);

    return mObj->create_compute_pipeline(mObj, pipelineI, pipelineObj);
}

void RDevice::destroy_pipeline(RPipeline pipeline)
{
    mObj->destroy_pipeline(mObj, pipeline);

    heap_delete((RPipelineObj*)pipeline);
}

void RDevice::update_set_images(uint32_t updateCount, const RSetImageUpdateInfo* updates)
{
    LD_PROFILE_SCOPE;

    mObj->update_set_images(mObj, updateCount, updates);
}

void RDevice::update_set_buffers(uint32_t updateCount, const RSetBufferUpdateInfo* updates)
{
    LD_PROFILE_SCOPE;

    mObj->update_set_buffers(mObj, updateCount, updates);
}

uint32_t RDevice::next_frame(RSemaphore& imageAcquired, RSemaphore& presentReady, RFence& frameComplete)
{
    LD_PROFILE_SCOPE;

    uint32_t framesInFlightCount = mObj->get_frames_in_flight_count(mObj);
    mObj->frameIndex = (mObj->frameIndex + 1) % framesInFlightCount;

    return mObj->next_frame(mObj, imageAcquired, presentReady, frameComplete);
}

void RDevice::present_frame()
{
    LD_PROFILE_SCOPE;

    return mObj->present_frame(mObj);
}

void RDevice::get_depth_stencil_formats(RFormat* formats, uint32_t& count)
{
    mObj->get_depth_stencil_formats(mObj, formats, count);
}

RSampleCountBit RDevice::get_max_sample_count()
{
    return mObj->get_max_sample_count(mObj);
}

RFormat RDevice::get_swapchain_color_format()
{
    return mObj->get_swapchain_color_format(mObj);
}

RImage RDevice::get_swapchain_color_attachment(uint32_t frameIdx)
{
    return mObj->get_swapchain_color_attachment(mObj, frameIdx);
}

uint32_t RDevice::get_swapchain_image_count()
{
    return mObj->get_swapchain_image_count(mObj);
}

void RDevice::get_swapchain_extent(uint32_t* width, uint32_t* height)
{
    return mObj->get_swapchain_extent(mObj, width, height);
}

uint32_t RDevice::get_frames_in_flight_count()
{
    return mObj->get_frames_in_flight_count(mObj);
}

uint32_t RDevice::get_frame_index()
{
    return mObj->frameIndex;
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

uint32_t RImage::layers() const
{
    return mObj->info.layers;
}

uint64_t RImage::size() const
{
    uint32_t texelSize = RUtil::get_format_texel_size(mObj->info.format);
    uint64_t layerSize = mObj->info.width * mObj->info.height * mObj->info.depth;

    return mObj->info.layers * layerSize * texelSize;
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

void* RBuffer::map_read(uint32_t offset, uint64_t size)
{
    LD_ASSERT(mObj->hostMap != nullptr);
    LD_ASSERT(offset + size <= mObj->info.size);

    return mObj->map_read(mObj, offset, size);
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

void RPipeline::set_color_write_mask(uint32_t index, RColorComponentFlags mask)
{
    RDeviceObj* deviceObj = mObj->deviceObj;

    deviceObj->pipeline_variant_color_write_mask(deviceObj, mObj, index, mask);
}

void RPipeline::set_depth_test_enable(bool enable)
{
    RDeviceObj* deviceObj = mObj->deviceObj;

    deviceObj->pipeline_variant_depth_test_enable(deviceObj, mObj, enable);
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
    // save pass information for later, used to invalidate graphics pipelines in cmd_bind_graphics_pipeline
    RUtil::save_pass_info(passBI.pass, mObj->currentPass);

    mObj->cmd_begin_pass(mObj, passBI);
}

void RCommandList::cmd_push_constant(const RPipelineLayoutInfo& layout, uint32_t offset, uint32_t size, const void* data)
{
    RPipelineLayoutObj* layoutObj = mObj->deviceObj->get_or_create_pipeline_layout_obj(layout);

    mObj->cmd_push_constant(mObj, layoutObj, offset, size, data);
}

void RCommandList::cmd_bind_graphics_pipeline(RPipeline pipeline)
{
    RPassInfo passI;
    RUtil::load_pass_info(mObj->currentPass, passI);

    RPipelineObj* pipelineObj = pipeline;

    // get or create graphics pipeline variant
    mObj->deviceObj->pipeline_variant_pass(mObj->deviceObj, pipelineObj, passI);
    pipelineObj->create_variant(pipelineObj);

    mObj->cmd_bind_graphics_pipeline(mObj, pipeline);
}

void RCommandList::cmd_bind_graphics_sets(const RPipelineLayoutInfo& layout, uint32_t firstSet, uint32_t setCount, RSet* sets)
{
    RPipelineLayoutObj* layoutObj = mObj->deviceObj->get_or_create_pipeline_layout_obj(layout);

    mObj->cmd_bind_graphics_sets(mObj, layoutObj, firstSet, setCount, sets);
}

void RCommandList::cmd_bind_compute_pipeline(RPipeline pipeline)
{
    mObj->cmd_bind_compute_pipeline(mObj, pipeline);
}

void RCommandList::cmd_bind_compute_sets(const RPipelineLayoutInfo& layout, uint32_t firstSet, uint32_t setCount, RSet* sets)
{
    RPipelineLayoutObj* layoutObj = mObj->deviceObj->get_or_create_pipeline_layout_obj(layout);

    mObj->cmd_bind_compute_sets(mObj, layoutObj, firstSet, setCount, sets);
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

void RCommandList::cmd_dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    mObj->cmd_dispatch(mObj, groupCountX, groupCountY, groupCountZ);
}

void RCommandList::cmd_draw(const RDrawInfo& drawI)
{
    mObj->cmd_draw(mObj, drawI);
}

void RCommandList::cmd_draw_indexed(const RDrawIndexedInfo& drawI)
{
    mObj->cmd_draw_indexed(mObj, drawI);
}

void RCommandList::cmd_end_pass()
{
    mObj->cmd_end_pass(mObj);
}

void RCommandList::cmd_buffer_memory_barrier(RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RBufferMemoryBarrier& barrier)
{
    mObj->cmd_buffer_memory_barrier(mObj, srcStages, dstStages, barrier);
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

void RCommandList::cmd_copy_image_to_buffer(RImage srcImage, RImageLayout srcImageLayout, RBuffer dstBuffer, uint32_t regionCount, const RBufferImageCopy* regions)
{
    LD_ASSERT(srcImage.usage() & RIMAGE_USAGE_TRANSFER_SRC_BIT);
    LD_ASSERT(dstBuffer.usage() & RBUFFER_USAGE_TRANSFER_DST_BIT);

    mObj->cmd_copy_image_to_buffer(mObj, srcImage, srcImageLayout, dstBuffer, regionCount, regions);
}

void RCommandList::cmd_blit_image(RImage srcImage, RImageLayout srcImageLayout, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RImageBlit* regions, RFilter filter)
{
    mObj->cmd_blit_image(mObj, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, regions, filter);
}

RCommandList RCommandPool::allocate()
{
    RCommandListObj* listObj = heap_new<RCommandListObj>(MEMORY_USAGE_RENDER);
    listObj->deviceObj = mObj->deviceObj;

    mObj->lists.push_back({listObj});

    return mObj->allocate(mObj, listObj);
}

void RCommandPool::reset()
{
    mObj->reset(mObj);
}

uint32_t hash32_pass_info(const RPassInfo& passI)
{
    std::string str = std::to_string(passI.colorAttachmentCount);

    str.push_back('m');
    str += std::to_string((int)passI.samples);

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

        if (passI.colorResolveAttachments)
        {
            const RPassResolveAttachment* resolve = passI.colorResolveAttachments + i;
            str.push_back('l');
            str += std::to_string((int)resolve->loadOp);
            str.push_back('s');
            str += std::to_string((int)resolve->storeOp);
            str.push_back('i');
            str += std::to_string((int)resolve->initialLayout);
            str.push_back('p');
            str += std::to_string((int)resolve->passLayout);
        }
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
    }

    if (passI.dependency)
    {
        const RPassDependency* dep = passI.dependency;
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
    std::size_t hash = (std::size_t)hash32_set_layout_info(layoutI.setLayouts[0]);

    for (uint32_t i = 1; i < layoutI.setLayoutCount; i++)
        hash_combine(hash, hash32_set_layout_info(layoutI.setLayouts[i]));

    // TODO: this truncates since std::size_t is most likely 8 bytes on 64-bit systems
    return (uint32_t)hash;
}

uint32_t hash32_framebuffer_info(const RFramebufferInfo& framebufferI)
{
    std::size_t hash = (std::size_t)hash32_pass_info(framebufferI.pass);

    // invalidation by size
    hash_combine(hash, framebufferI.width);
    hash_combine(hash, framebufferI.height);

    // invalidation by any referenced attachments
    for (uint32_t i = 0; i < framebufferI.colorAttachmentCount; i++)
    {
        hash_combine(hash, framebufferI.colorAttachments[i].rid());

        if (framebufferI.colorResolveAttachments)
            hash_combine(hash, framebufferI.colorResolveAttachments[i].rid());
    }

    if (framebufferI.depthStencilAttachment)
        hash_combine(hash, framebufferI.depthStencilAttachment.rid());

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

RSet RSetPool::allocate()
{
    RSetObj* setObj = (RSetObj*)mObj->setLA.allocate(sizeof(RSetObj));

    return mObj->allocate(mObj, setObj);
}

void RSetPool::reset()
{
    mObj->setLA.free();

    return mObj->reset(mObj);
}

RPassObj* RDeviceObj::get_or_create_pass_obj(const RPassInfo& passI)
{
    uint32_t passHash = hash32_pass_info(passI);

    if (!sPasses.contains(passHash))
    {
        RPassObj* passObj = (RPassObj*)heap_malloc(sizeof(RPassObj), MEMORY_USAGE_RENDER);
        passObj->rid = RObjectID::get();
        passObj->hash = passHash;
        passObj->colorAttachmentCount = passI.colorAttachmentCount;
        passObj->hasDepthStencilAttachment = passI.depthStencilAttachment != nullptr;
        passObj->samples = passI.samples;
        this->create_pass(this, passI, passObj);
        sPasses[passHash] = passObj;
    }

    return sPasses[passHash];
}

RSetLayoutObj* RDeviceObj::get_or_create_set_layout_obj(const RSetLayoutInfo& layoutI)
{
    uint32_t layoutHash = hash32_set_layout_info(layoutI);

    if (!sSetLayouts.contains(layoutHash))
    {
        RSetLayoutObj* layoutObj = (RSetLayoutObj*)heap_malloc(sizeof(RSetLayoutObj), MEMORY_USAGE_RENDER);
        layoutObj->rid = RObjectID::get();
        layoutObj->hash = layoutHash;
        layoutObj->deviceObj = this;
        this->create_set_layout(this, layoutI, layoutObj);
        sSetLayouts[layoutHash] = layoutObj;
    }

    return sSetLayouts[layoutHash];
}

RPipelineLayoutObj* RDeviceObj::get_or_create_pipeline_layout_obj(const RPipelineLayoutInfo& layoutI)
{
    LD_ASSERT(layoutI.setLayoutCount <= PIPELINE_LAYOUT_MAX_RESOURCE_SETS);

    uint32_t layoutHash = hash32_pipeline_layout_info(layoutI);

    if (!sPipelineLayouts.contains(layoutHash))
    {
        RPipelineLayoutObj* layoutObj = (RPipelineLayoutObj*)heap_malloc(sizeof(RPipelineLayoutObj), MEMORY_USAGE_RENDER);
        layoutObj->rid = RObjectID::get();
        layoutObj->hash = layoutHash;
        layoutObj->setCount = layoutI.setLayoutCount;
        for (uint32_t i = 0; i < layoutObj->setCount; i++)
            layoutObj->setLayoutObjs[i] = this->get_or_create_set_layout_obj(layoutI.setLayouts[i]);
        this->create_pipeline_layout(this, layoutI, layoutObj);
        sPipelineLayouts[layoutHash] = layoutObj;
    }

    return sPipelineLayouts[layoutHash];
}

RFramebufferObj* RDeviceObj::get_or_create_framebuffer_obj(const RFramebufferInfo& framebufferI)
{
    uint32_t framebufferHash = hash32_framebuffer_info(framebufferI);

    if (!sFramebuffers.contains(framebufferHash))
    {
        RFramebufferObj* framebufferObj = (RFramebufferObj*)heap_malloc(sizeof(RFramebufferObj), MEMORY_USAGE_RENDER);
        framebufferObj->rid = RObjectID::get();
        framebufferObj->hash = framebufferHash;
        framebufferObj->width = framebufferI.width;
        framebufferObj->height = framebufferI.height;
        framebufferObj->passObj = get_or_create_pass_obj(framebufferI.pass);
        create_framebuffer(this, framebufferI, framebufferObj);
        sFramebuffers[framebufferHash] = framebufferObj;

        for (uint32_t i = 0; i < framebufferI.colorAttachmentCount; i++)
        {
            RImageObj* imageObj = framebufferI.colorAttachments[i];
            imageObj->fboHashes.insert(framebufferHash);
        }

        if (framebufferI.depthStencilAttachment)
        {
            RImageObj* imageObj = (RImageObj*)((RImage)framebufferI.depthStencilAttachment);
            imageObj->fboHashes.insert(framebufferHash);
        }

        if (framebufferI.colorResolveAttachments)
        {
            for (uint32_t i = 0; i < framebufferI.colorAttachmentCount; i++)
            {
                RImageObj* imageObj = framebufferI.colorResolveAttachments[i];
                imageObj->fboHashes.insert(framebufferHash);
            }
        }
    }

    return sFramebuffers[framebufferHash];
}

} // namespace LD
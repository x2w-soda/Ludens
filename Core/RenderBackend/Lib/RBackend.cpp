#include "RBackendObj.h"
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
    return mObj->create_buffer(mObj, bufferI);
}

void RDevice::destroy_buffer(RBuffer buffer)
{
    mObj->destroy_buffer(mObj, buffer);
}

RImage RDevice::create_image(const RImageInfo& imageI)
{
    return mObj->create_image(mObj, imageI);
}

void RDevice::destroy_image(RImage image)
{
    mObj->destroy_image(mObj, image);
}

RPass RDevice::create_pass(const RPassInfo& passI)
{
    return mObj->create_pass(mObj, passI);
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

RQueue RDevice::get_graphics_queue()
{
    return mObj->get_graphics_queue(mObj);
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

void RCommandList::cmd_draw(const RDrawInfo& drawI)
{
    mObj->cmd_draw(mObj, drawI);
}

void RCommandList::cmd_end_pass()
{
    mObj->cmd_end_pass(mObj);
}

RCommandList RCommandPool::allocate()
{
    // NOTE: command pools (and its allocated command buffers) are never
    //       shared among threads, so this counter doesn't have to be atomic
    mObj->commandBufferCount++;

    return mObj->allocate(mObj);
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

uint32_t RSetLayout::hash() const
{
    return mObj->hash;
}

uint32_t RPipelineLayout::hash() const
{
    return mObj->hash;
}

} // namespace LD
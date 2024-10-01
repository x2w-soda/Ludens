#include "Core/RenderBase/Include/VK/VKInfo.h"
#include "Core/RenderBase/Lib/RDeviceVK.h"
#include "Core/RenderBase/Lib/RTextureVK.h"
#include "Core/RenderBase/Lib/RDeriveVK.h"
#include "Core/DSA/Include/Vector.h"
#include "Core/OS/Include/Time.h"
#include <iostream>

namespace LD
{

static RDeviceVK sDevice;

RDeviceVK::RDeviceVK()
{
}

RDeviceVK::~RDeviceVK()
{
    LD_DEBUG_ASSERT(ID == 0);
}

RResult RDeviceVK::CreateRenderDevice(RDevice& deviceH, const RDeviceInfo& info)
{
    LD_DEBUG_ASSERT((UID)sDevice.ID == 0 && "multi device is not yet implemented");
    LD_DEBUG_ASSERT(info.Backend == RBackend::Vulkan);

    sDevice.Startup(deviceH, info);

    return {};
}

RResult RDeviceVK::DeleteRenderDevice(RDevice& deviceH)
{
    LD_DEBUG_ASSERT((UID)sDevice.ID == (UID)deviceH);

    sDevice.Cleanup(deviceH);

    return {};
}

void RDeviceVK::Startup(RDevice& deviceH, const RDeviceInfo& info)
{
    RDeviceBase::Startup(deviceH, info);
    deviceH.mBackend = RBackend::Vulkan;

    TextureAllocator.Startup(MAX_TEXTURE_COUNT);
    BufferAllocator.Startup(MAX_BUFFER_COUNT);
    ShaderAllocator.Startup(MAX_SHADER_COUNT);
    BindingGroupLayoutAllocator.Startup(MAX_BINDING_GROUP_LAYOUT_COUNT);
    BindingGroupAllocator.Startup(MAX_BINDING_GROUP_COUNT);
    RenderPassAllocator.Startup(MAX_RENDER_PASS_COUNT);
    FrameBufferAllocator.Startup(MAX_FRAME_BUFFER_COUNT);
    PipelineAllocator.Startup(MAX_PIPELINE_COUNT);

    Context.Startup(VKContextInfo{});
    VKDevice& vkDevice = Context.GetDevice();
    VKSwapChain& vkSwapChain = Context.GetSwapChain();
    VkExtent2D vkSwapChainExtent = vkSwapChain.GetExtent();
    VkFormat vkSwapChainFormat = vkSwapChain.GetFormat();
    VkFormat vkDepthStencilFormat = Context.GetDepthStencilFormat();

    // see RDeviceVK::OnObserverNotify
    vkSwapChain.AddObserver(this);

    // create depth stencil image
    {
        RTextureFormat depthStencilFormat = DeriveRTextureFormat(vkDepthStencilFormat);

        RTextureInfo textureI;
        textureI.Type = RTextureType::Texture2D;
        textureI.Data = nullptr;
        textureI.Size = GetTextureFormatPixelSize(depthStencilFormat) * vkSwapChainExtent.width * vkSwapChainExtent.height;
        textureI.Format = depthStencilFormat;
        textureI.TextureUsage = RTextureUsageFlags::TEXTURE_USAGE_FRAME_BUFFER_ATTACHMENT_BIT;
        textureI.Width = vkSwapChainExtent.width;
        textureI.Height = vkSwapChainExtent.height;
        CreateTexture(DepthStencilAttachment, textureI);
    }

    {
        // NOTE: overkill and inaccurate

        Array<VkDescriptorPoolSize, 2> poolSizes = {
            VKInfo::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, DEVICE_CONCURRENT_FRAMES * MAX_BUFFER_COUNT),
            VKInfo::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                       DEVICE_CONCURRENT_FRAMES * MAX_TEXTURE_COUNT),
        };

        VkDescriptorPoolCreateInfo descriptorPoolCI = VKInfo::DescriptorPoolCreate(
            poolSizes.Size(), poolSizes.Data(), DEVICE_CONCURRENT_FRAMES * MAX_BINDING_GROUP_COUNT,
            VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

        DescriptorPool.Startup(vkDevice, descriptorPoolCI);
    }

    {
        VkCommandPoolCreateInfo commandPoolCI =
            VKInfo::CommandPoolCreate(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, vkDevice.GetGraphicsIndex());

        GraphicsCommandPool.Startup(vkDevice, commandPoolCI);

        commandPoolCI =
            VKInfo::CommandPoolCreate(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, vkDevice.GetTransferIndex());

        TransferCommandPool.Startup(vkDevice, commandPoolCI);
    }

    {
        Array<RPassAttachment, 1> attachments;
        attachments[0].Format = DeriveRTextureFormat(vkSwapChainFormat);
        attachments[0].InitialState = RState::Undefined;
        attachments[0].FinalState = RState::Present;
        attachments[0].LoadOp = RLoadOp::Clear;
        attachments[0].StoreOp = RStoreOp::Store;

        RPassInfo passI;
        passI.Name = "SwapChainRenderPass";
        passI.Attachments = attachments.GetView();
        CreateRenderPass(SwapChainRenderPass, passI);
    }

    const Vector<Ref<VKImageView>>& imageViews = vkSwapChain.GetImageViews();
    SwapChainImages.Resize(imageViews.Size());
    FrameBuffers.Resize(imageViews.Size());

    for (size_t i = 0; i < FrameBuffers.Size(); i++)
    {
        CreateTexture(SwapChainImages[i], imageViews[i]);

        RFrameBufferInfo frameBufferI{};
        frameBufferI.Width = vkSwapChainExtent.width;
        frameBufferI.Height = vkSwapChainExtent.height;
        frameBufferI.RenderPass = SwapChainRenderPass;
        frameBufferI.ColorAttachments = { 1, &SwapChainImages[i] };
        frameBufferI.DepthStencilAttachment.Reset();
        CreateFrameBuffer(FrameBuffers[i], frameBufferI);
    }

    for (FrameData& frame : Frames)
    {
        VkFenceCreateInfo fenceCI = VKInfo::FenceCreate(VK_FENCE_CREATE_SIGNALED_BIT);
        frame.Fence.FrameComplete.Startup(vkDevice, fenceCI);
        frame.Semaphore.ImageAvailable.Startup(vkDevice);
        frame.Semaphore.RenderComplete.Startup(vkDevice);
        frame.CommandBuffer.AllocatePrimary(vkDevice, GraphicsCommandPool, 1);
    }
}

void RDeviceVK::Cleanup(RDevice& deviceH)
{
    RDeviceBase::Cleanup(deviceH);

    VKDevice& vkDevice = Context.GetDevice();

    for (FrameData& frame : Frames)
    {
        frame.CommandBuffer.Free(vkDevice);
        frame.Semaphore.RenderComplete.Cleanup();
        frame.Semaphore.ImageAvailable.Cleanup();
        frame.Fence.FrameComplete.Cleanup();
    }

    LD_DEBUG_ASSERT(FrameBuffers.Size() == SwapChainImages.Size());

    for (size_t i = 0; i < FrameBuffers.Size(); i++)
    {
        DeleteFrameBuffer(FrameBuffers[i]);
        DeleteTexture(SwapChainImages[i]);
    }

    FrameBuffers.Clear();
    SwapChainImages.Clear();

    DeleteRenderPass(SwapChainRenderPass);

    TransferCommandPool.Cleanup();
    GraphicsCommandPool.Cleanup();
    DescriptorPool.Cleanup();

    // cleanup depth stencil image
    {
        DeleteTexture(DepthStencilAttachment);
    }

    Context.Cleanup();

    PipelineAllocator.Cleanup();
    FrameBufferAllocator.Cleanup();
    RenderPassAllocator.Cleanup();
    BindingGroupAllocator.Cleanup();
    BindingGroupLayoutAllocator.Cleanup();
    ShaderAllocator.Cleanup();
    BufferAllocator.Cleanup();
    TextureAllocator.Cleanup();
}

RResult RDeviceVK::CreateTexture(RTexture& textureH, Ref<VKImageView> view)
{
    RTextureVK* texture = (RTextureVK*)TextureAllocator.Alloc(sizeof(RTextureVK));
    new (texture) RTextureVK{};
    texture->Startup(textureH, view, *this);

    return {};
}

RResult RDeviceVK::CreateTexture(RTexture& textureH, const RTextureInfo& info)
{
    RTextureVK* texture = (RTextureVK*)TextureAllocator.Alloc(sizeof(RTextureVK));
    new (texture) RTextureVK{};
    texture->Startup(textureH, info, *this);

    return {};
}

RResult RDeviceVK::DeleteTexture(RTexture& textureH)
{
    RTextureVK& texture = Derive<RTextureVK>(textureH);

    texture.Cleanup(textureH);
    texture.~RTextureVK();
    TextureAllocator.Free(&texture);

    return {};
}

RResult RDeviceVK::CreateBuffer(RBuffer& bufferH, const RBufferInfo& info)
{
    RBufferVK* buffer = (RBufferVK*)BufferAllocator.Alloc(sizeof(RBufferVK));
    new (buffer) RBufferVK{};
    buffer->Startup(bufferH, info, *this);

    return {};
}

RResult RDeviceVK::DeleteBuffer(RBuffer& bufferH)
{
    RBufferVK& buffer = Derive<RBufferVK>(bufferH);

    VKDevice& vkDevice = Context.GetDevice();
    vkDeviceWaitIdle(vkDevice.GetHandle());

    buffer.Cleanup(bufferH);
    buffer.~RBufferVK();
    BufferAllocator.Free(&buffer);

    return {};
}

RResult RDeviceVK::CreateShader(RShader& shaderH, const RShaderInfo& info)
{
    RShaderVK* shader = (RShaderVK*)ShaderAllocator.Alloc(sizeof(RShaderVK));
    new (shader) RShaderVK{};
    shader->Startup(shaderH, info, *this);

    return {};
}

RResult RDeviceVK::DeleteShader(RShader& shaderH)
{
    RShaderVK& shader = Derive<RShaderVK>(shaderH);

    shader.Cleanup(shaderH);
    shader.~RShaderVK();
    ShaderAllocator.Free(&shader);

    return {};
}

RResult RDeviceVK::CreateBindingGroupLayout(RBindingGroupLayout& layoutH, const RBindingGroupLayoutInfo& info)
{
    RBindingGroupLayoutVK* layout =
        (RBindingGroupLayoutVK*)BindingGroupLayoutAllocator.Alloc(sizeof(RBindingGroupLayoutVK));
    new (layout) RBindingGroupLayoutVK{};
    layout->Startup(layoutH, info, *this);

    return {};
}

RResult RDeviceVK::DeleteBindingGroupLayout(RBindingGroupLayout& layoutH)
{
    RBindingGroupLayoutVK& layout = Derive<RBindingGroupLayoutVK>(layoutH);

    layout.Cleanup(layoutH);
    layout.~RBindingGroupLayoutVK();
    BindingGroupLayoutAllocator.Free(&layout);

    return {};
}

RResult RDeviceVK::CreateBindingGroup(RBindingGroup& groupH, const RBindingGroupInfo& info)
{
    RBindingGroupVK* group = (RBindingGroupVK*)BindingGroupAllocator.Alloc(sizeof(RBindingGroupVK));
    new (group) RBindingGroupVK{};
    group->Startup(groupH, info, *this);

    return {};
}

RResult RDeviceVK::DeleteBindingGroup(RBindingGroup& groupH)
{
    RBindingGroupVK& group = Derive<RBindingGroupVK>(groupH);

    group.Cleanup(groupH);
    group.~RBindingGroupVK();
    BindingGroupAllocator.Free(&group);

    return {};
}

RResult RDeviceVK::CreateRenderPass(RPass& passH, const RPassInfo& info)
{
    RPassVK* pass = (RPassVK*)RenderPassAllocator.Alloc(sizeof(RPassVK));
    new (pass) RPassVK{};
    pass->Startup(passH, info, *this);

    return {};
}

RResult RDeviceVK::DeleteRenderPass(RPass& passH)
{
    RPassVK& pass = Derive<RPassVK>(passH);

    pass.Cleanup(passH);
    pass.~RPassVK();
    RenderPassAllocator.Free(&pass);

    return {};
}

RResult RDeviceVK::CreateFrameBuffer(RFrameBuffer& frameBufferH, const RFrameBufferInfo& info)
{
    RFrameBufferVK* frameBuffer = (RFrameBufferVK*)FrameBufferAllocator.Alloc(sizeof(RFrameBufferVK));
    new (frameBuffer) RFrameBufferVK{};
    frameBuffer->Startup(frameBufferH, info, *this);

    return {};
}

RResult RDeviceVK::DeleteFrameBuffer(RFrameBuffer& frameBufferH)
{
    RFrameBufferVK& frameBuffer = Derive<RFrameBufferVK>(frameBufferH);

    frameBuffer.Cleanup(frameBufferH);
    frameBuffer.~RFrameBufferVK();
    FrameBufferAllocator.Free(&frameBuffer);

    return {};
}

RResult RDeviceVK::CreatePipeline(RPipeline& pipelineH, const RPipelineInfo& info)
{
    RPipelineVK* pipeline = (RPipelineVK*)PipelineAllocator.Alloc(sizeof(RPipelineVK));
    new (pipeline) RPipelineVK{};
    pipeline->Startup(pipelineH, info, *this);

    return {};
}

RResult RDeviceVK::DeletePipeline(RPipeline& pipelineH)
{
    RPipelineVK& pipeline = Derive<RPipelineVK>(pipelineH);

    VKDevice& vkDevice = Context.GetDevice();
    vkDeviceWaitIdle(vkDevice.GetHandle());

    pipeline.Cleanup(pipelineH);
    pipeline.~RPipelineVK();
    PipelineAllocator.Free(&pipeline);

    return {};
}

RResult RDeviceVK::GetSwapChainTextureFormat(RTextureFormat& format)
{
    VKSwapChain& vkSwapChain = Context.GetSwapChain();
    format = DeriveRTextureFormat(vkSwapChain.GetFormat());

    return {};
}

RResult RDeviceVK::GetSwapChainRenderPass(RPass& renderPass)
{
    LD_DEBUG_ASSERT(SwapChainRenderPass);
    renderPass = SwapChainRenderPass;

    return {};
}

RResult RDeviceVK::GetSwapChainFrameBuffer(RFrameBuffer& frameBuffer)
{
    RFrameBuffer currentFrameBuffer = FrameBuffers[ImageIndex];

    LD_DEBUG_ASSERT(currentFrameBuffer);
    frameBuffer = currentFrameBuffer;

    return {};
}

RResult RDeviceVK::BeginFrame()
{
    VKSwapChain& swapChain = Context.GetSwapChain();
    FrameData& frame = Frames[FrameIndex];

    frame.Fence.FrameComplete.Wait(UINT64_MAX);

    double swapChainWaitTime;
    {
        ScopeTimer timer(&swapChainWaitTime);
        VkResult result;
        ImageIndex = swapChain.AcquireImage((VkSemaphore)frame.Semaphore.ImageAvailable, result);
        if (result != VK_SUCCESS)
            return {}; // do not reset the FrameComplete fence, we have no image to write to, and no work to submit
    }

    frame.Fence.FrameComplete.Reset();

    frame.CommandBuffer.Reset(0);
    frame.CommandBuffer.BeginRecord(0);

    return {};
}

RResult RDeviceVK::EndFrame()
{
    VKDevice& device = Context.GetDevice();
    VKSwapChain& swapChain = Context.GetSwapChain();
    FrameData& frame = Frames[FrameIndex];

    frame.CommandBuffer.EndRecord();

    {
        VkCommandBuffer submission = frame.CommandBuffer.GetHandle();

        VkSubmitInfo submitInfo{};
        VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSemaphore waitSemaphore = frame.Semaphore.ImageAvailable.GetHandle();
        VkSemaphore signalSemaphore = frame.Semaphore.RenderComplete.GetHandle();
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitSemaphore;
        submitInfo.pWaitDstStageMask = &waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &submission;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphore;

        VK_ASSERT(vkQueueSubmit(device.GetGraphicsQueue(), 1, &submitInfo, (VkFence)frame.Fence.FrameComplete));
    }

    swapChain.PresentImage((VkSemaphore)frame.Semaphore.RenderComplete, ImageIndex);

    FrameIndex = (FrameIndex + 1) % Frames.Size();

    return {};
}

RResult RDeviceVK::BeginRenderPass(const RPassBeginInfo& info)
{
    VKRenderPass& pass = Derive<RPassVK>(info.RenderPass).RenderPass;
    VKFrameBuffer& frameBuffer = Derive<RFrameBufferVK>(info.FrameBuffer).FrameBuffer;
    VKSwapChain& vkSwapChain = Context.GetSwapChain();
    FrameData& frame = Frames[FrameIndex];

    Vector<VkClearValue> vkClearValues(info.ClearValues.Size());
    for (size_t i = 0; i < info.ClearValues.Size(); i++)
    {
        if (info.ClearValues[i].Color.HasValue())
        {
            const RClearColorValue& value = info.ClearValues[i].Color.Value();
            vkClearValues[i].color = { value.r, value.g, value.b, value.a };
        }
        else if (info.ClearValues[i].DepthStencil.HasValue())
        {
            const RClearDepthStencilValue& value = info.ClearValues[i].DepthStencil.Value();
            vkClearValues[i].depthStencil.depth = (float)value.Depth;
            vkClearValues[i].depthStencil.stencil = (uint32_t)value.Stencil;
        }
        else
            LD_DEBUG_UNREACHABLE;
    }

    VkRenderPassBeginInfo beginInfo = VKInfo::RenderPassBegin(
        pass.GetHandle(), frameBuffer.GetHandle(), vkSwapChain.GetExtent(), vkClearValues.Size(), vkClearValues.Data());

    frame.CommandBuffer.CmdBeginRenderPass(beginInfo);

    return {};
}

RResult RDeviceVK::EndRenderPass()
{
    FrameData& frame = Frames[FrameIndex];

    frame.CommandBuffer.CmdEndRenderPass();

    return {};
}

RResult RDeviceVK::SetPipeline(RPipeline& pipelineH)
{
    FrameData& frame = Frames[FrameIndex];
    VKPipeline& pipeline = Derive<RPipelineVK>(pipelineH).Pipeline;
    VkExtent2D swapChainExtent = Context.GetSwapChain().GetExtent();
    VkRect2D scissor = VKInfo::Rect2D(swapChainExtent);
    VkViewport viewport;

    viewport.x = 0.0f;
    viewport.y = (float)swapChainExtent.height;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = -(float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    frame.CommandBuffer.CmdBindGraphicsPipeline(pipeline.GetHandle());
    frame.CommandBuffer.CmdSetViewport(viewport);
    frame.CommandBuffer.CmdSetScissor(scissor);

    return {};
}

RResult RDeviceVK::SetBindingGroup(u32 groupIdx, RBindingGroup& groupH)
{
    FrameData& frame = Frames[FrameIndex];
    VkDescriptorSet descriptorSet = Derive<RBindingGroupVK>(groupH).DescriptorSet.GetHandle();
    VkPipelineLayout pipelineLayout = Derive<RPipelineVK>(BoundPipelineH).PipelineLayout.GetHandle();

    vkCmdBindDescriptorSets(frame.CommandBuffer.GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, groupIdx,
                            1, &descriptorSet, 0, nullptr);

    return {};
}

RResult RDeviceVK::SetVertexBuffer(u32 slot, RBuffer& bufferH)
{
    FrameData& frame = Frames[FrameIndex];
    VKBuffer& buffer = Derive<RBufferVK>(bufferH).Buffer;

    frame.CommandBuffer.CmdBindVertexBuffer(slot, buffer.GetHandle(), 0);

    return {};
}

RResult RDeviceVK::SetIndexBuffer(RBuffer& bufferH, RIndexType indexType)
{
    FrameData& frame = Frames[FrameIndex];
    VKBuffer& buffer = Derive<RBufferVK>(bufferH).Buffer;

    VkIndexType vkIndexType = DeriveVKIndexType(indexType);
    frame.CommandBuffer.CmdBindIndexBuffer(buffer.GetHandle(), 0, vkIndexType);

    return {};
}

RResult RDeviceVK::PushScissor(const Rect2D& scissor)
{
    FrameData& frame = Frames[FrameIndex];

    Scissors.Push(scissor);

    VkRect2D vkScissor;
    vkScissor.extent.width = scissor.w;
    vkScissor.extent.height = scissor.h;
    vkScissor.offset.x = scissor.x;
    vkScissor.offset.y = scissor.y;
    frame.CommandBuffer.CmdSetScissor(vkScissor);
    
    return {};
}

RResult RDeviceVK::PopScissor()
{
    LD_DEBUG_ASSERT(!Scissors.IsEmpty());

    FrameData& frame = Frames[FrameIndex];
    VkRect2D vkScissor;

    Scissors.Pop();

    if (!Scissors.IsEmpty())
    {
        const Rect2D scissor = Scissors.Top();
        vkScissor.extent.width = scissor.w;
        vkScissor.extent.height = scissor.h;
        vkScissor.offset.x = scissor.x;
        vkScissor.offset.y = scissor.y;
    }
    else
    {
        vkScissor.extent.width = ViewportExtent.x;
        vkScissor.extent.height = ViewportExtent.y;
        vkScissor.offset.x = 0;
        vkScissor.offset.y = 0;
    }

    frame.CommandBuffer.CmdSetScissor(vkScissor);

    return {};
}

RResult RDeviceVK::DrawVertex(const RDrawVertexInfo& info)
{
    FrameData& frame = Frames[FrameIndex];

    LD_DEBUG_ASSERT(info.VertexStart == 0);
    LD_DEBUG_ASSERT(info.InstanceStart == 0);
    frame.CommandBuffer.CmdDrawVertex(info.VertexCount, info.InstanceCount);

    return {};
}

RResult RDeviceVK::DrawIndexed(const RDrawIndexedInfo& info)
{
    FrameData& frame = Frames[FrameIndex];

    LD_DEBUG_ASSERT(info.InstanceStart == 0);
    frame.CommandBuffer.CmdDrawIndexed(info.IndexCount, info.InstanceCount, info.IndexStart);

    return {};
}

RResult RDeviceVK::ResizeViewport(int width, int height)
{
    VKSwapChain& vkSwapChain = Context.GetSwapChain();

    VKSwapChainConfig config = vkSwapChain.GetConfig();
    config.SwapExtent.width = (uint32_t)width;
    config.SwapExtent.height = (uint32_t)height;

    vkSwapChain.Invalidate(config);

    return {};
}

void RDeviceVK::WaitIdle()
{
    VKDevice& vkDevice = Context.GetDevice();

    vkDeviceWaitIdle(vkDevice.GetHandle());
}

void RDeviceVK::OnObserverNotify(Observable<VKSwapChainInvalidation>* swapchain,
                                 const VKSwapChainInvalidation& newConfig)
{
    VKSwapChain& vkSwapChain = *static_cast<VKSwapChain*>(swapchain);
    VkExtent2D vkSwapChainExtent = newConfig.SwapExtent;

    // recreate resources
    const Vector<Ref<VKImageView>>& imageViews = vkSwapChain.GetImageViews();
    SwapChainImages.Resize(imageViews.Size());
    FrameBuffers.Resize(imageViews.Size());

    for (size_t i = 0; i < FrameBuffers.Size(); i++)
    {
        DeleteTexture(SwapChainImages[i]);
        CreateTexture(SwapChainImages[i], imageViews[i]);

        RFrameBufferInfo frameBufferI{};
        frameBufferI.Width = vkSwapChainExtent.width;
        frameBufferI.Height = vkSwapChainExtent.height;
        frameBufferI.RenderPass = SwapChainRenderPass;
        frameBufferI.ColorAttachments = { 1, &SwapChainImages[i] };
        frameBufferI.DepthStencilAttachment.Reset();
        DeleteFrameBuffer(FrameBuffers[i]);
        CreateFrameBuffer(FrameBuffers[i], frameBufferI);
    }
}

} // namespace LD
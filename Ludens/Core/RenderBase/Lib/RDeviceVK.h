#pragma once

#include "Core/Header/Include/Observer.h"
#include "Core/DSA/Include/Array.h"
#include "Core/RenderBase/Include/VK/VKContext.h"
#include "Core/RenderBase/Include/VK/VKCommand.h"
#include "Core/RenderBase/Include/VK/VKFence.h"
#include "Core/RenderBase/Include/VK/VKSemaphore.h"
#include "Core/RenderBase/Lib/RTextureVK.h"
#include "Core/RenderBase/Lib/RBufferVK.h"
#include "Core/RenderBase/Lib/RShaderVK.h"
#include "Core/RenderBase/Lib/RBindingVK.h"
#include "Core/RenderBase/Lib/RPassVK.h"
#include "Core/RenderBase/Lib/RFrameBufferVK.h"
#include "Core/RenderBase/Lib/RPipelineVK.h"
#include "Core/RenderBase/Lib/RBase.h"

#define DEVICE_CONCURRENT_FRAMES 2

namespace LD
{

struct RDeviceVK : RDeviceBase, public Observer<VKSwapChainInvalidation>
{
    RDeviceVK();
    RDeviceVK(const RDeviceVK&) = delete;
    ~RDeviceVK();

    RDeviceVK& operator=(const RDeviceVK&) = delete;

    static RResult CreateRenderDevice(RDevice& deviceH, const RDeviceInfo& info);
    static RResult DeleteRenderDevice(RDevice& deviceH);

    virtual void Startup(RDevice& deviceH, const RDeviceInfo& info);
    virtual void Cleanup(RDevice& deviceH);

    RResult CreateTexture(RTexture& texture, Ref<VKImageView> view);

    virtual RResult CreateTexture(RTexture& texture, const RTextureInfo& info) override;
    virtual RResult DeleteTexture(RTexture& texture) override;

    virtual RResult CreateBuffer(RBuffer& buffer, const RBufferInfo& info) override;
    virtual RResult DeleteBuffer(RBuffer& buffer) override;

    virtual RResult CreateShader(RShader& shader, const RShaderInfo& info) override;
    virtual RResult DeleteShader(RShader& shader) override;

    virtual RResult CreateBindingGroupLayout(RBindingGroupLayout& layoutH,
                                             const RBindingGroupLayoutInfo& info) override;
    virtual RResult DeleteBindingGroupLayout(RBindingGroupLayout& layoutH) override;

    virtual RResult CreateBindingGroup(RBindingGroup& groupH, const RBindingGroupInfo& info) override;
    virtual RResult DeleteBindingGroup(RBindingGroup& groupH) override;

    virtual RResult CreateRenderPass(RPass& passH, const RPassInfo& info) override;
    virtual RResult DeleteRenderPass(RPass& passH) override;

    virtual RResult CreateFrameBuffer(RFrameBuffer& frameBufferH, const RFrameBufferInfo& info) override;
    virtual RResult DeleteFrameBuffer(RFrameBuffer& frameBufferH) override;

    virtual RResult CreatePipeline(RPipeline& pipeline, const RPipelineInfo& info) override;
    virtual RResult DeletePipeline(RPipeline& pipeline) override;

    virtual RResult GetSwapChainTextureFormat(RTextureFormat& format) override;
    virtual RResult GetSwapChainRenderPass(RPass& renderPass) override;
    virtual RResult GetSwapChainFrameBuffer(RFrameBuffer& frameBuffer) override;

    virtual RResult BeginFrame() override;
    virtual RResult EndFrame() override;
    virtual RResult BeginRenderPass(const RPassBeginInfo& info) override;
    virtual RResult EndRenderPass() override;

    virtual RResult SetPipeline(RPipeline& pipeline) override;
    virtual RResult SetBindingGroup(u32 groupIdx, RBindingGroup& groupH) override;
    virtual RResult SetVertexBuffer(u32 slot, RBuffer& buffer) override;
    virtual RResult SetIndexBuffer(RBuffer& buffer, RIndexType indexType) override;

    virtual RResult PushScissor(const Rect2D& scissor) override;
    virtual RResult PopScissor() override;

    virtual RResult DrawVertex(const RDrawVertexInfo& info) override;
    virtual RResult DrawIndexed(const RDrawIndexedInfo& info) override;

    virtual RResult ResizeViewport(int width, int height) override;

    virtual void WaitIdle() override;

    virtual void OnObserverNotify(Observable<VKSwapChainInvalidation>* swapchain,
                                  const VKSwapChainInvalidation& newConfig) override;

    // TODO: this assumes a single device on a single thread, needs refactoring later for multi-threading
    VKContext Context;
    VKDescriptorPool DescriptorPool;
    VKCommandPool GraphicsCommandPool;
    VKCommandPool TransferCommandPool;

    // depth stencil attachment needs to be explicitly created
    RTexture DepthStencilAttachment;

    RPass SwapChainRenderPass;
    Vector<RTexture> SwapChainImages;
    Vector<RFrameBuffer> FrameBuffers; // one frame buffer per swap chain image

    struct FrameData
    {
        struct
        {
            VKFence FrameComplete;
        } Fence;

        struct
        {
            VKSemaphore RenderComplete;
            VKSemaphore ImageAvailable;
        } Semaphore;

        VKCommandBuffer CommandBuffer;
    };

    Array<FrameData, DEVICE_CONCURRENT_FRAMES> Frames;
    int FrameIndex;
    int ImageIndex;

    PoolAllocator<sizeof(RTextureVK)> TextureAllocator;
    PoolAllocator<sizeof(RBufferVK)> BufferAllocator;
    PoolAllocator<sizeof(RShaderVK)> ShaderAllocator;
    PoolAllocator<sizeof(RBindingGroupLayoutVK)> BindingGroupLayoutAllocator;
    PoolAllocator<sizeof(RBindingGroupVK)> BindingGroupAllocator;
    PoolAllocator<sizeof(RPassVK)> RenderPassAllocator;
    PoolAllocator<sizeof(RFrameBufferVK)> FrameBufferAllocator;
    PoolAllocator<sizeof(RPipelineVK)> PipelineAllocator;
};

} // namespace LD
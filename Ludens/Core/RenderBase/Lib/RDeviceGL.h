#pragma once

#include "Core/OS/Include/UID.h"
#include "Core/OS/Include/Allocator.h"
#include "Core/Header/Include/Error.h"
#include "Core/RenderBase/Include/GL/GLContext.h"
#include "Core/RenderBase/Include/GL/GLVertex.h"
#include "Core/RenderBase/Lib/RTextureGL.h"
#include "Core/RenderBase/Lib/RBufferGL.h"
#include "Core/RenderBase/Lib/RShaderGL.h"
#include "Core/RenderBase/Lib/RBindingGL.h"
#include "Core/RenderBase/Lib/RPassGL.h"
#include "Core/RenderBase/Lib/RFrameBufferGL.h"
#include "Core/RenderBase/Lib/RPipelineGL.h"
#include "Core/RenderBase/Lib/RBase.h"

namespace LD
{

struct RDeviceGL : RDeviceBase
{
    RDeviceGL();
    RDeviceGL(const RDeviceGL&) = delete;
    ~RDeviceGL();

    RDeviceGL& operator=(const RDeviceGL&) = delete;

    static RResult CreateRenderDevice(RDevice& deviceH, const RDeviceInfo& info);
    static RResult DeleteRenderDevice(RDevice& deviceH);

    virtual void Startup(RDevice& deviceH, const RDeviceInfo& info);
    virtual void Cleanup(RDevice& deviceH);

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

    /// create a handle referencing the default frame buffer created along OpenGL context
    RResult CreateDefaultFrameBuffer(RFrameBuffer& frameBufferH);

    /// delete a handle referencing the default frame buffer
    RResult DeleteDefaultFrameBuffer(RFrameBuffer& frameBufferH);

    PoolAllocator<sizeof(RTextureGL)> TextureAllocator;
    PoolAllocator<sizeof(RBufferGL)> BufferAllocator;
    PoolAllocator<sizeof(RShaderGL)> ShaderAllocator;
    PoolAllocator<sizeof(RBindingGroupLayoutGL)> BindingGroupLayoutAllocator;
    PoolAllocator<sizeof(RBindingGroupGL)> BindingGroupAllocator;
    PoolAllocator<sizeof(RPassGL)> RenderPassAllocator;
    PoolAllocator<sizeof(RFrameBufferGL)> FrameBufferAllocator;
    PoolAllocator<sizeof(RPipelineGL)> PipelineAllocator;
    GLContext Context;
    GLenum IndexType;

    RPass DefaultRenderPass;
    RFrameBuffer DefaultFrameBuffer;
};

} // namespace LD
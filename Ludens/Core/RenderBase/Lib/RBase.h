#pragma once

#include <string>
#include "Core/DSA/Include/Array.h"
#include "Core/DSA/Include/Stack.h"
#include "Core/RenderBase/Include/RTexture.h"
#include "Core/RenderBase/Include/RBuffer.h"
#include "Core/RenderBase/Include/RShader.h"
#include "Core/RenderBase/Include/RBinding.h"
#include "Core/RenderBase/Include/RPass.h"
#include "Core/RenderBase/Include/RFrameBuffer.h"
#include "Core/RenderBase/Include/RPipeline.h"

// TODO: randomly hard coded values here, consolidate with graphihcs API backend to determine actual limit
#define MAX_TEXTURE_COUNT 1024
#define MAX_BUFFER_COUNT 1024
#define MAX_SHADER_COUNT 1024
#define MAX_BINDING_GROUP_LAYOUT_COUNT 512
#define MAX_BINDING_GROUP_COUNT 512
#define MAX_RENDER_PASS_COUNT 256
#define MAX_FRAME_BUFFER_COUNT 256
#define MAX_PIPELINE_COUNT 512

namespace LD
{

template <typename THandle, typename TBase = typename THandle::Base>
inline TBase& Unwrap(const THandle& handle)
{
    LD_DEBUG_ASSERT((bool)handle);

    return *static_cast<TBase*>(handle);
}

template <typename TDerived, typename THandle, typename TBase = typename THandle::Base>
inline TDerived& Derive(const THandle& handle)
{
    LD_DEBUG_ASSERT((bool)handle);

    TDerived* derived = dynamic_cast<TDerived*>((TBase*)handle);

    LD_DEBUG_ASSERT(derived != nullptr);
    return *derived;
}

struct RDeviceBase
{
    RDeviceBase() = default;
    RDeviceBase(const RDeviceBase&) = delete;
    virtual ~RDeviceBase();

    RDeviceBase& operator=(const RDeviceBase&) = delete;

    void Startup(RDevice& deviceH, const RDeviceInfo& info);
    void Cleanup(RDevice& deviceH);

    virtual RResult CreateTexture(RTexture& texture, const RTextureInfo& info) = 0;
    virtual RResult DeleteTexture(RTexture& texture) = 0;

    virtual RResult CreateBuffer(RBuffer& buffer, const RBufferInfo& info) = 0;
    virtual RResult DeleteBuffer(RBuffer& buffer) = 0;

    virtual RResult CreateShader(RShader& shader, const RShaderInfo& info) = 0;
    virtual RResult DeleteShader(RShader& shader) = 0;

    virtual RResult CreateBindingGroupLayout(RBindingGroupLayout& layout, const RBindingGroupLayoutInfo& info) = 0;
    virtual RResult DeleteBindingGroupLayout(RBindingGroupLayout& layout) = 0;

    virtual RResult CreateBindingGroup(RBindingGroup& group, const RBindingGroupInfo& info) = 0;
    virtual RResult DeleteBindingGroup(RBindingGroup& group) = 0;

    virtual RResult CreateRenderPass(RPass& passH, const RPassInfo& info) = 0;
    virtual RResult DeleteRenderPass(RPass& passH) = 0;

    virtual RResult CreateFrameBuffer(RFrameBuffer& frameBufferH, const RFrameBufferInfo& info) = 0;
    virtual RResult DeleteFrameBuffer(RFrameBuffer& frameBufferH) = 0;

    virtual RResult CreatePipeline(RPipeline& pipeline, const RPipelineInfo& info) = 0;
    virtual RResult DeletePipeline(RPipeline& pipeline) = 0;

    virtual RResult GetSwapChainTextureFormat(RTextureFormat& format) = 0;
    virtual RResult GetSwapChainRenderPass(RPass& renderPass) = 0;
    virtual RResult GetSwapChainFrameBuffer(RFrameBuffer& frameBuffer) = 0;

    virtual RResult BeginFrame() = 0;
    virtual RResult EndFrame() = 0;
    virtual RResult BeginRenderPass(const RPassBeginInfo& info) = 0;
    virtual RResult EndRenderPass() = 0;

    virtual RResult SetPipeline(RPipeline& pipeline) = 0;
    virtual RResult SetBindingGroup(u32 slot, RBindingGroup& group) = 0;
    virtual RResult SetVertexBuffer(u32 slot, RBuffer& buffer) = 0;
    virtual RResult SetIndexBuffer(RBuffer& buffer, RIndexType indexType) = 0;

    virtual RResult PushScissor(const Rect2D& scissor) = 0;
    virtual RResult PopScissor() = 0;

    virtual RResult DrawVertex(const RDrawVertexInfo& info) = 0;
    virtual RResult DrawIndexed(const RDrawIndexedInfo& info) = 0;

    virtual RResult ResizeViewport(int width, int height) = 0;

    virtual void WaitIdle() {}

    CUID<RDeviceBase> ID;
    Stack<Rect2D> Scissors;
    Vec2 ViewportExtent;
    RDrawStats* Stats = nullptr;
    RResultCallback Callback;
    RPipeline BoundPipelineH;
    RPass CurrentPassH;
};

struct RTextureBase
{
    RTextureBase() = default;
    RTextureBase(const RTextureBase&) = delete;
    virtual ~RTextureBase();

    RTextureBase& operator=(const RTextureBase&) = delete;

    void Startup(RTexture& textureH, RDeviceBase* device);
    void Startup(RTexture& textureH, const RTextureInfo& info, RDeviceBase* device);
    void Cleanup(RTexture& textureH);

    CUID<RTextureBase> ID;
    RDeviceBase* Device = nullptr;
};

struct RBufferBase
{
    RBufferBase() = default;
    RBufferBase(const RBufferBase&) = delete;
    virtual ~RBufferBase();

    RBufferBase& operator=(const RBufferBase&) = delete;

    void Startup(RBuffer& bufferH, const RBufferInfo& info, RDeviceBase* device);
    void Cleanup(RBuffer& bufferH);

    virtual RResult SetData(u32 offset, u32 size, const void* data) = 0;

    CUID<RBufferBase> ID;
    RDeviceBase* Device = nullptr;
    RBufferType Type;
};

struct RShaderBase
{
    RShaderBase() = default;
    RShaderBase(const RShaderBase&) = delete;
    virtual ~RShaderBase();

    RShaderBase& operator=(const RShaderBase&) = delete;

    void Startup(RShader& shaderH, const RShaderInfo& info, RDeviceBase* device);
    void Cleanup(RShader& shaderH);

    CUID<RShaderBase> ID;
    RDeviceBase* Device = nullptr;
    RShaderSourceType SourceType;
    RShaderType Type;
};

struct RBindingGroupLayoutBase
{
    RBindingGroupLayoutBase() = default;
    RBindingGroupLayoutBase(const RBindingGroupLayoutBase&) = delete;
    virtual ~RBindingGroupLayoutBase();

    RBindingGroupLayoutBase& operator=(const RBindingGroupLayoutBase&) = delete;

    void Startup(RBindingGroupLayout& layoutH, const RBindingGroupLayoutInfo& info, RDeviceBase* device);
    void Cleanup(RBindingGroupLayout& layoutH);

    inline size_t GetBindingCount() const
    {
        return Bindings.Size();
    }

    inline bool HasSameLayout(const RBindingGroupLayoutBase& other)
    {
        if (Bindings.Size() != other.Bindings.Size())
            return false;

        for (size_t i = 0; i < Bindings.Size(); i++)
            if (Bindings[i].Type != other.Bindings[i].Type)
                return false;

        return true;
    }

    CUID<RBindingGroupLayoutBase> ID;
    RDeviceBase* Device = nullptr;
    Vector<RBindingInfo> Bindings;
};

struct RBindingGroupBase
{
    RBindingGroupBase() = default;
    RBindingGroupBase(const RBindingGroupBase&) = delete;
    virtual ~RBindingGroupBase();

    RBindingGroupBase& operator=(const RBindingGroupBase&) = delete;

    void Startup(RBindingGroup& groupH, const RBindingGroupInfo& info, RDeviceBase* device);
    void Cleanup(RBindingGroup& groupH);

    virtual RResult BindTexture(u32 binding, RTexture& textureH, int arrayIndex) = 0;
    virtual RResult BindUniformBuffer(u32 binding, RBuffer& bufferH) = 0;

    struct Binding
    {
        RBindingType Type;
        Vector<RTexture> TextureH;
        RBuffer BufferH;
    };

    CUID<RBindingGroupBase> ID;
    RDeviceBase* Device = nullptr;
    Vector<Binding> Bindings;
    RBindingGroupLayout GroupLayoutH;
};

struct RPassBase
{
    RPassBase() = default;
    RPassBase(const RPassBase&) = delete;
    virtual ~RPassBase();

    RPassBase& operator=(const RPassBase&) = delete;

    void Startup(RPass& passH, const RPassInfo& info, RDeviceBase* device);
    void Cleanup(RPass& passH);

    bool HasDepthStencilAttachment() const;

    CUID<RPassBase> ID;
    RDeviceBase* Device = nullptr;
    Vector<RPassAttachment> Attachments;
};

struct RFrameBufferBase
{
    RFrameBufferBase() = default;
    RFrameBufferBase(const RFrameBufferBase&) = delete;
    virtual ~RFrameBufferBase();

    RFrameBufferBase& operator=(const RFrameBufferBase&) = delete;

    void ReadInfo(const RFrameBufferInfo& info);

    void Startup(RFrameBuffer& frameBufferH, const RFrameBufferInfo& info, RDeviceBase* device);
    void Cleanup(RFrameBuffer& frameBufferH);

    virtual RResult Invalidate(const RFrameBufferInfo& info) = 0;

    RResult GetColorAttachment(int idx, RTexture* colorAttachment);
    RResult GetDepthStencilAttachment(RTexture* depthStencilAttachment);

    CUID<RFrameBufferBase> ID;
    RDeviceBase* Device = nullptr;
    u32 Width = 0;
    u32 Height = 0;
    Optional<RTexture> DepthStencilAttachment;
    Vector<RTexture> ColorAttachments;
};

struct RPipelineBase
{
    RPipelineBase() = default;
    RPipelineBase(const RPipelineBase&) = delete;
    virtual ~RPipelineBase();

    RPipelineBase& operator=(const RPipelineBase&) = delete;

    void Startup(RPipeline& pipelineH, const RPipelineInfo& info, RDeviceBase* device);
    void Cleanup(RPipeline& pipelineH);

    CUID<RPipelineBase> ID;
    RDeviceBase* Device = nullptr;
    std::string Name;
    RVertexLayout VertexLayout;
    RShader VertexShaderH;
    RShader FragmentShaderH;
    Vector<RBindingGroupLayout> GroupLayoutsH;

    bool DepthTestEnabled;
    bool DepthWriteEnabled;
    RCompareMode DepthCompareMode;
    RCullMode CullMode;
    RPolygonMode PolygonMode;
};

} // namespace LD
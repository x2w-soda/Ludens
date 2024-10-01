#pragma once

#include "Core/Header/Include/Types.h"
#include "Core/Math/Include/Rect2D.h"
#include "Core/OS/Include/UID.h"
#include "Core/DSA/Include/Optional.h"
#include "Core/RenderBase/Include/RTypes.h"
#include "Core/RenderBase/Include/RResult.h"

namespace LD
{

class RDevice;
class RTexture;
class RBuffer;
class RShader;
class RBindingGroupLayout;
class RBindingGroup;
class RPass;
class RFrameBuffer;
class RPipeline;
struct RDeviceBase;
struct RTextureInfo;
struct RBufferInfo;
struct RShaderInfo;
struct RBindingGroupLayoutInfo;
struct RBindingGroupInfo;
struct RPassInfo;
struct RPassBeginInfo;
struct RFrameBufferInfo;
struct RPipelineInfo;
enum class RTextureFormat;

using RResultCallback = void (*)(const RResult&);

struct RDeviceInfo
{
    RBackend Backend;
    RResultCallback Callback = nullptr;
};

RResult CreateRenderDevice(RDevice& device, const RDeviceInfo& info);
RResult DeleteRenderDevice(RDevice& device);

struct RDrawStats
{
    u32 DrawVertexCalls;
    u32 DrawIndexedCalls;

    // Every draw call increments this metric by (Instance Count) * (Vertex / Index Count).
    // Note that per-instance vertices are *NOT* included.
    u32 TotalVertices;

    inline u32 DrawCalls() const
    {
        return DrawVertexCalls + DrawIndexedCalls;
    }
};

// render device handle and interface
class RDevice : public RHandle<RDeviceBase>
{
    friend struct RDeviceGL;
    friend struct RDeviceVK;

public:

    RResult CreateTexture(RTexture& texture, const RTextureInfo& info);
    RResult DeleteTexture(RTexture& texture);

    RResult CreateBuffer(RBuffer& buffer, const RBufferInfo& info);
    RResult DeleteBuffer(RBuffer& buffer);

    RResult CreateShader(RShader& shader, const RShaderInfo& info);
    RResult DeleteShader(RShader& shader);

    RResult CreateBindingGroupLayout(RBindingGroupLayout& layout, const RBindingGroupLayoutInfo& info);
    RResult DeleteBindingGroupLayout(RBindingGroupLayout& layout);

    RResult CreateBindingGroup(RBindingGroup& group, const RBindingGroupInfo& info);
    RResult DeleteBindingGroup(RBindingGroup& group);

    RResult CreateRenderPass(RPass& passH, const RPassInfo& info);
    RResult DeleteRenderPass(RPass& passH);

    RResult CreateFrameBuffer(RFrameBuffer& frameBufferH, const RFrameBufferInfo& info);
    RResult DeleteFrameBuffer(RFrameBuffer& frameBufferH);

    RResult CreatePipeline(RPipeline& pipeline, const RPipelineInfo& info);
    RResult DeletePipeline(RPipeline& pipeline);

    RResult GetSwapChainTextureFormat(RTextureFormat& format);
    RResult GetSwapChainRenderPass(RPass& renderPass);
    RResult GetSwapChainFrameBuffer(RFrameBuffer& frameBuffer);

    // NOTE: Temporary API for single render device in a single thread.
    //       Will refactor later once we introduce job systems and render graphs for multi-threading.

    RResult BeginFrame();
    RResult EndFrame();
    RResult BeginRenderPass(const RPassBeginInfo& info);
    RResult EndRenderPass();

    RResult SetPipeline(RPipeline& pipeline);
    RResult SetBindingGroup(u32 slot, RBindingGroup& group);
    RResult SetVertexBuffer(u32 slot, RBuffer& buffer);
    RResult SetIndexBuffer(RBuffer& buffer, RIndexType indexType);

    // TODO: Multiple drawstats overlapping on one device.
    RResult BeginDrawStats(RDrawStats* stats);
    RResult EndDrawStats();

    RResult PushScissor(const Rect2D& scissor);
    RResult PopScissor();

    RResult DrawVertex(const RDrawVertexInfo& info);
    RResult DrawIndexed(const RDrawIndexedInfo& info);

    /// @brief resize the screen viewport
    /// @param width pixel width of the viewport
    /// @param height pixel height of the viewport
    /// @return resize result
    RResult ResizeViewport(int width, int height);

    void WaitIdle();

    inline RBackend GetBackend() const
    {
        return mBackend;
    }

private:
    RBackend mBackend;
};

} // namespace LD
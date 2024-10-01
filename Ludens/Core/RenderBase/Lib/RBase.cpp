#include <cstring>
#include "Core/Header/Include/Error.h"
#include "Core/RenderBase/Lib/RBase.h"
#include "Core/RenderBase/Include/RTexture.h"

namespace LD
{

///
/// Device Base
///

RDeviceBase::~RDeviceBase()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RDeviceBase::Startup(RDevice& deviceH, const RDeviceInfo& info)
{
    ID = CUID<RDeviceBase>::Get();
    Callback = info.Callback ? info.Callback : [](const RResult&) {};

    deviceH.SetHandle(ID, this);
}

void RDeviceBase::Cleanup(RDevice& deviceH)
{
    ID.Reset();

    deviceH.ResetHandle();
}

///
/// Texture Base
///

RTextureBase::~RTextureBase()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RTextureBase::Startup(RTexture& textureH, RDeviceBase* device)
{
    ID = CUID<RTextureBase>::Get();
    Device = device;

    textureH.SetHandle(ID, this);
}

void RTextureBase::Startup(RTexture& textureH, const RTextureInfo& info, RDeviceBase* device)
{
    (void)info;

    Startup(textureH, device);
}

void RTextureBase::Cleanup(RTexture& textureH)
{
    ID.Reset();
    Device = nullptr;

    textureH.ResetHandle();
}

///
/// Buffer Base
///

RBufferBase::~RBufferBase()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RBufferBase::Startup(RBuffer& bufferH, const RBufferInfo& info, RDeviceBase* device)
{
    ID = CUID<RBufferBase>::Get();
    Device = device;
    Type = info.Type;

    bufferH.SetHandle(ID, this);
}

void RBufferBase::Cleanup(RBuffer& bufferH)
{
    ID.Reset();
    Device = nullptr;

    bufferH.ResetHandle();
}

///
/// Shader Base
///

RShaderBase::~RShaderBase()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RShaderBase::Startup(RShader& shaderH, const RShaderInfo& info, RDeviceBase* device)
{
    ID = CUID<RShaderBase>::Get();
    Device = device;
    SourceType = info.SourceType;
    Type = info.Type;

    shaderH.SetHandle(ID, this);
}

void RShaderBase::Cleanup(RShader& shaderH)
{
    ID.Reset();
    Device = nullptr;

    shaderH.ResetHandle();
}

///
/// Binding Group Layout Base
///

RBindingGroupLayoutBase::~RBindingGroupLayoutBase()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RBindingGroupLayoutBase::Startup(RBindingGroupLayout& layoutH, const RBindingGroupLayoutInfo& info,
                                      RDeviceBase* device)
{
    ID = CUID<RBindingGroupLayoutBase>::Get();
    Device = device;

    // copy binding information
    size_t bindingCount = info.Bindings.Size();
    Bindings.Resize(bindingCount);
    for (size_t i = 0; i < bindingCount; i++)
    {
        Bindings[i] = info.Bindings[i];
    }

    layoutH.SetHandle(ID, this);
}

void RBindingGroupLayoutBase::Cleanup(RBindingGroupLayout& layoutH)
{
    ID.Reset();
    Device = nullptr;

    layoutH.ResetHandle();
}

///
/// Binding Group Base
///

RBindingGroupBase::~RBindingGroupBase()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RBindingGroupBase::Startup(RBindingGroup& groupH, const RBindingGroupInfo& info, RDeviceBase* device)
{
    ID = CUID<RBindingGroupBase>::Get();
    Device = device;

    GroupLayoutH = info.Layout;
    RBindingGroupLayoutBase& layout = *(RBindingGroupLayoutBase*)GroupLayoutH;

    size_t bindingCount = layout.Bindings.Size();
    Bindings.Resize(bindingCount);

    for (size_t i = 0; i < bindingCount; i++)
    {
        Bindings[i].Type = layout.Bindings[i].Type;

        if (Bindings[i].Type == RBindingType::Texture)
            Bindings[i].TextureH.Resize(layout.Bindings[i].Count);
    }

    groupH.SetHandle(ID, this);
}

void RBindingGroupBase::Cleanup(RBindingGroup& groupH)
{
    ID.Reset();
    Device = nullptr;

    groupH.ResetHandle();
}

///
/// Render Pass Base
///

RPassBase::~RPassBase()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RPassBase::Startup(RPass& passH, const RPassInfo& info, RDeviceBase* device)
{
    ID = CUID<RPassBase>::Get();
    Device = device;

    Attachments.Resize(info.Attachments.Size());
    std::copy(info.Attachments.Begin(), info.Attachments.End(), Attachments.Begin());

    passH.SetHandle(ID, this);
}

void RPassBase::Cleanup(RPass& passH)
{
    ID.Reset();
    Device = nullptr;

    passH.ResetHandle();
}

bool RPassBase::HasDepthStencilAttachment() const
{
    for (const RPassAttachment& attachment : Attachments)
    {
        if (IsDepthStencilTextureFormat(attachment.Format))
            return true;
    }

    return false;
}

///
/// Frame Buffer Base
///

RFrameBufferBase::~RFrameBufferBase()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RFrameBufferBase::ReadInfo(const RFrameBufferInfo& info)
{
    Width = info.Width;
    Height = info.Height;

    ColorAttachments.Resize(info.ColorAttachments.Size());
    std::copy(info.ColorAttachments.Begin(), info.ColorAttachments.End(), ColorAttachments.Begin());

    DepthStencilAttachment = info.DepthStencilAttachment;
}

void RFrameBufferBase::Startup(RFrameBuffer& frameBufferH, const RFrameBufferInfo& info, RDeviceBase* device)
{
    ID = CUID<RFrameBufferBase>::Get();

    Device = device;
    ReadInfo(info);

    LD_DEBUG_ASSERT(Width > 0 && Height > 0);

    frameBufferH.SetHandle(ID, this);
}

void RFrameBufferBase::Cleanup(RFrameBuffer& frameBufferH)
{
    ID.Reset();
    Device = nullptr;

    frameBufferH.ResetHandle();
}

RResult RFrameBufferBase::GetColorAttachment(int idx, RTexture* colorAttachment)
{
    RResult result{};

    if (idx < 0 || idx >= ColorAttachments.Size())
    {
        result.Type = RResultType::InvalidIndex;
        return result;
    }

    *colorAttachment = ColorAttachments[idx];

    return result;
}

RResult RFrameBufferBase::GetDepthStencilAttachment(RTexture* depthStencilAttachment)
{
    // TODO: RResult error code
    LD_DEBUG_ASSERT(DepthStencilAttachment.HasValue());

    *depthStencilAttachment = DepthStencilAttachment.Value();

    return {};
}

///
/// Pipeline Base
///

RPipelineBase::~RPipelineBase()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RPipelineBase::Startup(RPipeline& pipelineH, const RPipelineInfo& info, RDeviceBase* device)
{
    ID = CUID<RPipelineBase>::Get();
    Device = device;

    if (info.Name)
        Name = { info.Name, strlen(info.Name) };

    VertexLayout = info.VertexLayout;
    VertexShaderH = info.VertexShader;
    FragmentShaderH = info.FragmentShader;
    DepthTestEnabled = info.DepthStencilState.DepthTestEnabled;
    DepthWriteEnabled = info.DepthStencilState.DepthWriteEnabled;
    DepthCompareMode = info.DepthStencilState.DepthCompareMode;
    CullMode = info.RasterizationState.CullMode;
    PolygonMode = info.RasterizationState.PolygonMode;

    const RPipelineLayout& pipelineLayout = info.PipelineLayout;

    GroupLayoutsH.Resize(pipelineLayout.GroupLayouts.Size());
    for (size_t i = 0; i < GroupLayoutsH.Size(); i++)
    {
        GroupLayoutsH[i] = pipelineLayout.GroupLayouts[i];
    }

    pipelineH.SetHandle(ID, this);
}

void RPipelineBase::Cleanup(RPipeline& pipelineH)
{
    ID.Reset();
    Device = nullptr;

    GroupLayoutsH.Clear();

    pipelineH.ResetHandle();
}

} // namespace LD
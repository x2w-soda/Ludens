#pragma once

#include <iostream>
#include "Core/RenderBase/Include/RDevice.h"
#include "Core/RenderBase/Include/RShader.h"
#include "Core/RenderBase/Include/RBuffer.h"
#include "Core/RenderBase/Include/RBinding.h"
#include "Core/RenderBase/Include/RPass.h"
#include "Core/RenderBase/Lib/RDeviceGL.h"
#include "Core/RenderBase/Lib/RDeviceVK.h"
#include "Core/RenderBase/Lib/RBindingGL.h"

namespace LD
{

static inline bool MissingPipeline(RDeviceBase* device, RResult& result)
{
    if (!device->BoundPipelineH)
    {
        result.Type = RResultType::ResourceMissing;
        result.ResourceMissing.MissingType = RResourceType::Pipeline;
        return true;
    }

    return false;
}

static inline bool BufferTypeMismatch(RBufferType expect, RBufferType actual, RResult& result)
{
    if (expect != actual)
    {
        result.Type = RResultType::BufferTypeMismatch;
        result.BufferTypeMismatch.Expect = expect;
        result.BufferTypeMismatch.Actual = actual;
        return true;
    }

    return false;
}

static inline bool ShaderTypeMismatch(RShaderType expect, RShaderType actual, RResult& result)
{
    if (expect != actual)
    {
        result.Type = RResultType::ShaderTypeMismatch;
        result.ShaderTypeMismatch.Expect = expect;
        result.ShaderTypeMismatch.Actual = actual;
        return true;
    }

    return false;
}

RResult CreateRenderDevice(RDevice& device, const RDeviceInfo& info)
{
    RResult result;

    if (device)
    {
        result.Type = RResultType::InvalidHandle;
        return result;
    }

    switch (info.Backend)
    {
    case RBackend::OpenGL:
        result = RDeviceGL::CreateRenderDevice(device, info);
        break;
    case RBackend::Vulkan:
        result = RDeviceVK::CreateRenderDevice(device, info);
        break;
    }

    return result;
}

RResult DeleteRenderDevice(RDevice& device)
{
    RResult result;

    if (!device)
    {
        result.Type = RResultType::InvalidHandle;
        return result;
    }

    switch (device.GetBackend())
    {
    case RBackend::OpenGL:
        result = RDeviceGL::DeleteRenderDevice(device);
        break;
    case RBackend::Vulkan:
        result = RDeviceVK::DeleteRenderDevice(device);
        break;
    }

    return result;
}

RResult RDevice::CreateTexture(RTexture& texture, const RTextureInfo& info)
{
    RResult result;

    size_t expectDataSize = GetTextureFormatPixelSize(info.Format) * info.Width * info.Height;
    if (info.Type == RTextureType::TextureCube)
        expectDataSize *= 6;

    if (texture)
        result.Type = RResultType::InvalidHandle;
    else if (info.Data != nullptr && info.Size != expectDataSize)
    {
        result.Type = RResultType::TextureSizeMismatch;
        result.TextureSizeMismatch.Expect = expectDataSize;
        result.TextureSizeMismatch.Actual = info.Size;
    }
    else
        result = mBase->CreateTexture(texture, info);

    mBase->Callback(result);
    return result;
}

RResult RDevice::DeleteTexture(RTexture& texture)
{
    RResult result;

    if (!texture)
        result.Type = RResultType::InvalidHandle;
    else
        result = mBase->DeleteTexture(texture);

    mBase->Callback(result);
    return result;
}

RResult RDevice::CreateBuffer(RBuffer& buffer, const RBufferInfo& info)
{
    RResult result;

    if (buffer)
        result.Type = RResultType::InvalidHandle;
    else
        result = mBase->CreateBuffer(buffer, info);

    mBase->Callback(result);
    return result;
}

RResult RDevice::DeleteBuffer(RBuffer& buffer)
{
    RResult result;

    if (!buffer)
        result.Type = RResultType::InvalidHandle;
    else
        result = mBase->DeleteBuffer(buffer);

    mBase->Callback(result);
    return result;
}

RResult RDevice::CreateShader(RShader& shader, const RShaderInfo& info)
{
    RResult result;

    if (shader)
        result.Type = RResultType::InvalidHandle;
    else
        result = mBase->CreateShader(shader, info);

    mBase->Callback(result);
    return result;
}

RResult RDevice::DeleteShader(RShader& shader)
{
    RResult result;

    if (!shader)
        result.Type = RResultType::InvalidHandle;
    else
        result = mBase->DeleteShader(shader);

    mBase->Callback(result);
    return result;
}

RResult RDevice::CreateBindingGroupLayout(RBindingGroupLayout& layout, const RBindingGroupLayoutInfo& info)
{
    RResult result;

    if (layout)
        result.Type = RResultType::InvalidHandle;
    else
        result = mBase->CreateBindingGroupLayout(layout, info);

    mBase->Callback(result);
    return result;
}

RResult RDevice::DeleteBindingGroupLayout(RBindingGroupLayout& layout)
{
    RResult result;

    if (!layout)
        result.Type = RResultType::InvalidHandle;
    else
        result = mBase->DeleteBindingGroupLayout(layout);

    mBase->Callback(result);
    return result;
}

RResult RDevice::CreateBindingGroup(RBindingGroup& group, const RBindingGroupInfo& info)
{
    RResult result;

    if (group)
        result.Type = RResultType::InvalidHandle;
    else
        result = mBase->CreateBindingGroup(group, info);

    mBase->Callback(result);
    return result;
}

RResult RDevice::DeleteBindingGroup(RBindingGroup& group)
{
    RResult result;

    if (!group)
        result.Type = RResultType::InvalidHandle;
    else
        result = mBase->DeleteBindingGroup(group);

    mBase->Callback(result);
    return result;
}

RResult RDevice::CreateRenderPass(RPass& pass, const RPassInfo& info)
{
    RResult result;

    if (pass)
        result.Type = RResultType::InvalidHandle;
    else
        result = mBase->CreateRenderPass(pass, info);

    mBase->Callback(result);
    return result;
}

RResult RDevice::DeleteRenderPass(RPass& pass)
{
    RResult result;

    LD_DEBUG_ASSERT(pass != mBase->CurrentPassH);

    if (!pass)
        result.Type = RResultType::InvalidHandle;
    else
        result = mBase->DeleteRenderPass(pass);

    mBase->Callback(result);
    return result;
}

RResult RDevice::CreateFrameBuffer(RFrameBuffer& frameBuffer, const RFrameBufferInfo& info)
{
    RResult result;

    if (frameBuffer)
        result.Type = RResultType::InvalidHandle;
    else
        result = mBase->CreateFrameBuffer(frameBuffer, info);

    mBase->Callback(result);
    return result;
}

RResult RDevice::DeleteFrameBuffer(RFrameBuffer& frameBuffer)
{
    RResult result;

    if (!frameBuffer)
        result.Type = RResultType::InvalidHandle;
    else
        result = mBase->DeleteFrameBuffer(frameBuffer);

    mBase->Callback(result);
    return result;
}

RResult RDevice::CreatePipeline(RPipeline& pipeline, const RPipelineInfo& info)
{
    RResult result;

    if (pipeline)
    {
        result.Type = RResultType::InvalidHandle;
        mBase->Callback(result);
        return result;
    }

    if (!info.VertexShader || !info.FragmentShader)
    {
        result.Type = RResultType::ResourceMissing;
        result.ResourceMissing.MissingType = RResourceType::Shader;
        mBase->Callback(result);
        return result;
    }

    RShaderBase& vs = Unwrap(info.VertexShader);
    RShaderBase& fs = Unwrap(info.FragmentShader);

    if (ShaderTypeMismatch(RShaderType::VertexShader, vs.Type, result) ||
        ShaderTypeMismatch(RShaderType::FragmentShader, fs.Type, result))
    {
        mBase->Callback(result);
        return result;
    }

    result = mBase->CreatePipeline(pipeline, info);
    mBase->Callback(result);
    return result;
}

RResult RDevice::DeletePipeline(RPipeline& pipeline)
{
    RResult result;

    if (!pipeline)
    {
        result.Type = RResultType::InvalidHandle;
        mBase->Callback(result);
        return result;
    }

    // if we are deleting the bound pipeline, unbind it first
    if (pipeline == mBase->BoundPipelineH)
    {
        mBase->BoundPipelineH.ResetHandle();
    }

    result = mBase->DeletePipeline(pipeline);
    mBase->Callback(result);
    return result;
}

RResult RDevice::GetSwapChainTextureFormat(RTextureFormat& format)
{
    RResult result;

    result = mBase->GetSwapChainTextureFormat(format);

    mBase->Callback(result);
    return {};
}

RResult RDevice::GetSwapChainRenderPass(RPass& renderPass)
{
    RResult result;

    result = mBase->GetSwapChainRenderPass(renderPass);

    mBase->Callback(result);
    return {};
}

RResult RDevice::GetSwapChainFrameBuffer(RFrameBuffer& frameBuffer)
{
    RResult result;

    result = mBase->GetSwapChainFrameBuffer(frameBuffer);

    mBase->Callback(result);
    return {};
}

RResult RDevice::BeginFrame()
{
    RResult result;

    result = mBase->BeginFrame();

    mBase->Callback(result);
    return result;
}

RResult RDevice::EndFrame()
{
    RResult result;

    result = mBase->EndFrame();

    mBase->Callback(result);
    return result;
}

RResult RDevice::BeginRenderPass(const RPassBeginInfo& info)
{
    RResult result;
    RPassBase& pass = Unwrap(info.RenderPass);

    int expectedClearValues = 0;
    for (RPassAttachment& attachment : pass.Attachments)
    {
        if (attachment.LoadOp == RLoadOp::Clear)
            expectedClearValues++;
    }

    if (expectedClearValues != info.ClearValues.Size())
    {
        result.Type = RResultType::PassBeginError;
        result.PassBeginError.MissingClearValueIndex = -1;
        result.PassBeginError.NumClearValuesExpect = expectedClearValues;
        result.PassBeginError.NumClearValuesActual = info.ClearValues.Size();
        mBase->Callback(result);
        return result;
    }

    int attachmentCount = (int)pass.Attachments.Size();

    for (int i = 0; i < attachmentCount; i++)
    {
        const RPassAttachment& attachment = pass.Attachments[i];
        bool isColorAttachment = IsColorTextureFormat(attachment.Format);
        bool isMissingClearValue = false;

        // check clear value for color attachment
        if (isColorAttachment && attachment.LoadOp == RLoadOp::Clear && !info.ClearValues[i].Color.HasValue())
            isMissingClearValue = true;

        // check clear value for depth stencil attachment
        if (!isColorAttachment && attachment.LoadOp == RLoadOp::Clear && !info.ClearValues[i].DepthStencil.HasValue())
            isMissingClearValue = true;

        if (isMissingClearValue)
        {
            result.Type = RResultType::PassBeginError;
            result.PassBeginError.MissingClearValueIndex = i;
            result.PassBeginError.NumClearValuesExpect = attachmentCount;
            result.PassBeginError.NumClearValuesActual = attachmentCount;
            mBase->Callback(result);
            return result;
        }
    }

    // TODO: check if framebuffer color attachments are in the ShaderResource state

    mBase->CurrentPassH = info.RenderPass;
    result = mBase->BeginRenderPass(info);

    mBase->Callback(result);
    return result;
}

RResult RDevice::EndRenderPass()
{
    RResult result;

    result = mBase->EndRenderPass();
    mBase->CurrentPassH.ResetHandle();

    mBase->Callback(result);
    return result;
}

RResult RDevice::SetPipeline(RPipeline& pipelineH)
{
    RResult result;
    RPipelineBase& pipeline = Unwrap(pipelineH);

    // NOTE: Render backends generally allow pipelines to perform depth-testing even if the frame buffer
    //       does not provide a depth stencil attachment (should be undefined behavior), this is just a
    //       sanity check that crashes debug builds only.
    LD_DEBUG_ASSERT(!(pipeline.DepthTestEnabled && !mBase->CurrentPassH.HasDepthStencilAttachment()));

    mBase->BoundPipelineH = pipelineH;
    result = mBase->SetPipeline(pipelineH);
    mBase->Callback(result);
    return result;
}

RResult RDevice::SetBindingGroup(u32 slot, RBindingGroup& groupH)
{
    RResult result;

    // compare binding group layout with pipeline layout
    if (mBase->BoundPipelineH)
    {
        RPipelineBase& pipeline = Unwrap(mBase->BoundPipelineH);
        RBindingGroupBase& group = Unwrap(groupH);
        RBindingGroupLayoutBase& layout = Unwrap(group.GroupLayoutH);
        size_t groupCount = pipeline.GroupLayoutsH.Size();

        if (slot >= groupCount)
        {
            result.Type = RResultType::InvalidIndex;
            mBase->Callback(result);
            return result;
        }

        RBindingGroupLayoutBase& slotLayout = Unwrap(pipeline.GroupLayoutsH[slot]);

        if (!layout.HasSameLayout(slotLayout))
        {
            result.Type = RResultType::BindingGroupMismatch;
            mBase->Callback(result);
            return result;
        }
    }

    result = mBase->SetBindingGroup(slot, groupH);
    mBase->Callback(result);
    return result;
}

RResult RDevice::SetVertexBuffer(u32 slot, RBuffer& bufferH)
{
    RResult result;

    if (MissingPipeline(mBase, result))
    {
        mBase->Callback(result);
        return result;
    }

    // TODO: bufferH invalid handle check?
    RBufferBase& buffer = Unwrap(bufferH);

    if (BufferTypeMismatch(RBufferType::VertexBuffer, buffer.Type, result))
    {
        mBase->Callback(result);
        return result;
    }

    RPipelineBase& pipeline = Unwrap(mBase->BoundPipelineH);
    size_t vertexBufferSlotCount = pipeline.VertexLayout.Slots.Size();

    if (slot >= vertexBufferSlotCount)
    {
        result.Type = RResultType::InvalidIndex;
        mBase->Callback(result);
        return result;
    }

    result = mBase->SetVertexBuffer(slot, bufferH);
    mBase->Callback(result);
    return result;
}

RResult RDevice::SetIndexBuffer(RBuffer& bufferH, RIndexType indexType)
{
    RResult result;

    if (MissingPipeline(mBase, result))
    {
        mBase->Callback(result);
        return result;
    }

    // TODO: bufferH invalid handle
    RBufferBase& buffer = Unwrap(bufferH);

    if (BufferTypeMismatch(RBufferType::IndexBuffer, buffer.Type, result))
    {
        mBase->Callback(result);
        return result;
    }

    result = mBase->SetIndexBuffer(bufferH, indexType);
    mBase->Callback(result);
    return result;
}

RResult RDevice::BeginDrawStats(RDrawStats* stats)
{
    RResult result;

    LD_DEBUG_ASSERT(stats != nullptr);
    stats->TotalVertices = 0;
    stats->DrawVertexCalls = 0;
    stats->DrawIndexedCalls = 0;

    mBase->Stats = stats;
    mBase->Callback(result);
    return result;
}

RResult RDevice::EndDrawStats()
{
    RResult result;

    mBase->Stats = nullptr;
    mBase->Callback(result);
    return result;
}

RResult RDevice::PushScissor(const Rect2D& scissor)
{
    RResult result;

    result = mBase->PushScissor(scissor);
    mBase->Callback(result);
    return result;
}

RResult RDevice::PopScissor()
{
    RResult result;

    if (mBase->Scissors.IsEmpty())
    {
        result.Type = RResultType::ScissorStackEmpty;
        return result;
    }

    result = mBase->PopScissor();
    mBase->Callback(result);
    return result;
}

RResult RDevice::DrawVertex(const RDrawVertexInfo& info)
{
    RResult result;

    if (MissingPipeline(mBase, result))
    {
        mBase->Callback(result);
        return result;
    }

    result = mBase->DrawVertex(info);
    if (result && mBase->Stats)
    {
        mBase->Stats->DrawVertexCalls++;
        mBase->Stats->TotalVertices += info.VertexCount * info.InstanceCount;
    }

    mBase->Callback(result);
    return result;
}

RResult RDevice::DrawIndexed(const RDrawIndexedInfo& info)
{
    RResult result;

    if (MissingPipeline(mBase, result))
    {
        mBase->Callback(result);
        return result;
    }

    result = mBase->DrawIndexed(info);
    if (result && mBase->Stats)
    {
        mBase->Stats->DrawIndexedCalls++;
        mBase->Stats->TotalVertices += info.IndexCount * info.InstanceCount;
    }

    mBase->Callback(result);
    return result;
}

RResult RDevice::ResizeViewport(int width, int height)
{
    RResult result = mBase->ResizeViewport(width, height);

    mBase->Callback(result);
    return result;
}

void RDevice::WaitIdle()
{
    mBase->WaitIdle();
}

} // namespace LD
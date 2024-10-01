#include <unordered_set>
#include "Core/DSA/Include/Array.h"
#include "Core/DSA/Include/Vector.h"
#include "Core/RenderBase/Include/GL/GL.h"
#include "Core/RenderBase/Lib/RDeriveGL.h"
#include "Core/RenderBase/Lib/RDeviceGL.h"
#include "Core/RenderBase/Lib/RTextureGL.h"
#include "Core/RenderBase/Lib/RBufferGL.h"
#include "Core/RenderBase/Lib/RShaderGL.h"
#include "Core/RenderBase/Lib/RPassGL.h"
#include "Core/RenderBase/Lib/RFrameBufferGL.h"
#include "Core/RenderBase/Lib/RBindingGL.h"
#include "Core/RenderBase/Lib/RPipelineGL.h"
#include "Core/RenderBase/Lib/RBase.h"

namespace LD
{

static RDeviceGL sDevice;

RDeviceGL::RDeviceGL()
{
}

RDeviceGL::~RDeviceGL()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RDeviceGL::Startup(RDevice& handle, const RDeviceInfo& info)
{
    RDeviceBase::Startup(handle, info);
    handle.mBackend = RBackend::OpenGL;

    TextureAllocator.Startup(MAX_TEXTURE_COUNT);
    BufferAllocator.Startup(MAX_BUFFER_COUNT);
    ShaderAllocator.Startup(MAX_SHADER_COUNT);
    BindingGroupLayoutAllocator.Startup(MAX_BINDING_GROUP_LAYOUT_COUNT);
    BindingGroupAllocator.Startup(MAX_BINDING_GROUP_COUNT);
    RenderPassAllocator.Startup(MAX_RENDER_PASS_COUNT);
    FrameBufferAllocator.Startup(MAX_FRAME_BUFFER_COUNT);
    PipelineAllocator.Startup(MAX_PIPELINE_COUNT);

    Context.Startup();

    // TODO: query default frame buffer texture format, guarantee
    //       that default render pass works with default frame buffer.

    Array<RPassAttachment, 1> attachments;
    attachments[0].Format = RTextureFormat::RGBA8;
    attachments[0].InitialState = RState::Undefined;
    attachments[0].FinalState = RState::Present;
    attachments[0].LoadOp = RLoadOp::Clear;
    attachments[0].StoreOp = RStoreOp::Store;

    RPassInfo passI;
    passI.Name = "SwapChainRenderPass";
    passI.Attachments = attachments.GetView();
    CreateRenderPass(DefaultRenderPass, passI);

    CreateDefaultFrameBuffer(DefaultFrameBuffer);
}

void RDeviceGL::Cleanup(RDevice& handle)
{
    RDeviceBase::Cleanup(handle);

    DeleteDefaultFrameBuffer(DefaultFrameBuffer);

    DeleteRenderPass(DefaultRenderPass);

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

RResult RDeviceGL::CreateRenderDevice(RDevice& deviceH, const RDeviceInfo& info)
{
    LD_DEBUG_ASSERT((UID)sDevice.ID == 0 && "multi device is not yet implemented");
    LD_DEBUG_ASSERT(info.Backend == RBackend::OpenGL);

    sDevice.Startup(deviceH, info);

    return {};
}

RResult RDeviceGL::DeleteRenderDevice(RDevice& deviceH)
{
    LD_DEBUG_ASSERT((UID)sDevice.ID == (UID)deviceH);

    sDevice.Cleanup(deviceH);

    return {};
}

RResult RDeviceGL::CreateTexture(RTexture& textureH, const RTextureInfo& info)
{
    RTextureGL* texture = (RTextureGL*)TextureAllocator.Alloc(sizeof(RTextureGL));
    new (texture) RTextureGL{};
    texture->Startup(textureH, info, *this);

    return {};
}

RResult RDeviceGL::DeleteTexture(RTexture& textureH)
{
    RTextureGL& texture = Derive<RTextureGL>(textureH);

    texture.Cleanup(textureH);
    texture.~RTextureGL();
    TextureAllocator.Free(&texture);

    return {};
}

RResult RDeviceGL::CreateBuffer(RBuffer& bufferH, const RBufferInfo& info)
{
    RBufferGL* buffer = (RBufferGL*)BufferAllocator.Alloc(sizeof(RBufferGL));
    new (buffer) RBufferGL{};
    buffer->Startup(bufferH, info, *this);

    return {};
}

RResult RDeviceGL::DeleteBuffer(RBuffer& bufferH)
{
    RBufferGL* buffer = dynamic_cast<RBufferGL*>((RBufferBase*)bufferH);

    buffer->Cleanup(bufferH);
    buffer->~RBufferGL();
    BufferAllocator.Free(buffer);

    return {};
}

RResult RDeviceGL::CreateShader(RShader& shaderH, const RShaderInfo& info)
{
    RShaderGL* shader = (RShaderGL*)ShaderAllocator.Alloc(sizeof(RShaderGL));
    new (shader) RShaderGL{};
    shader->Startup(shaderH, info, *this);

    return {};
}

RResult RDeviceGL::DeleteShader(RShader& shaderH)
{
    RShaderGL* shader = dynamic_cast<RShaderGL*>((RShaderBase*)shaderH);

    shader->Cleanup(shaderH);
    shader->~RShaderGL();
    ShaderAllocator.Free(shader);

    return {};
}

RResult RDeviceGL::CreateBindingGroupLayout(RBindingGroupLayout& layoutH, const RBindingGroupLayoutInfo& info)
{
    RBindingGroupLayoutGL* layout =
        (RBindingGroupLayoutGL*)BindingGroupLayoutAllocator.Alloc(sizeof(RBindingGroupLayoutGL));
    new (layout) RBindingGroupLayoutGL{};
    layout->Startup(layoutH, info, *this);

    return {};
}

RResult RDeviceGL::DeleteBindingGroupLayout(RBindingGroupLayout& layoutH)
{
    RBindingGroupLayoutGL* layout = dynamic_cast<RBindingGroupLayoutGL*>((RBindingGroupLayoutBase*)layoutH);

    layout->Cleanup(layoutH);
    layout->~RBindingGroupLayoutGL();
    BindingGroupLayoutAllocator.Free(layout);

    return {};
}

RResult RDeviceGL::CreateBindingGroup(RBindingGroup& groupH, const RBindingGroupInfo& info)
{
    RBindingGroupGL* group = (RBindingGroupGL*)BindingGroupAllocator.Alloc(sizeof(RBindingGroupGL));
    new (group) RBindingGroupGL{};
    group->Startup(groupH, info, *this);

    return {};
}

RResult RDeviceGL::DeleteBindingGroup(RBindingGroup& groupH)
{
    RBindingGroupGL* group = dynamic_cast<RBindingGroupGL*>((RBindingGroupBase*)groupH);

    group->Cleanup(groupH);
    group->~RBindingGroupGL();
    BindingGroupAllocator.Free(group);

    return {};
}

RResult RDeviceGL::CreateRenderPass(RPass& passH, const RPassInfo& info)
{
    RPassGL* pass = (RPassGL*)RenderPassAllocator.Alloc(sizeof(RPassGL));
    new (pass) RPassGL{};
    pass->Startup(passH, info, *this);

    return {};
}

RResult RDeviceGL::DeleteRenderPass(RPass& passH)
{
    RPassGL& pass = Derive<RPassGL>(passH);
    pass.Cleanup(passH);
    pass.~RPassGL();
    RenderPassAllocator.Free(&pass);

    return {};
}

RResult RDeviceGL::CreateFrameBuffer(RFrameBuffer& frameBufferH, const RFrameBufferInfo& info)
{
    RFrameBufferGL* frameBuffer = (RFrameBufferGL*)FrameBufferAllocator.Alloc(sizeof(RFrameBufferGL));
    new (frameBuffer) RFrameBufferGL{};
    frameBuffer->Startup(frameBufferH, info, *this);

    return {};
}

RResult RDeviceGL::DeleteFrameBuffer(RFrameBuffer& frameBufferH)
{
    RFrameBufferGL& frameBuffer = Derive<RFrameBufferGL>(frameBufferH);

    frameBuffer.Cleanup(frameBufferH);
    frameBuffer.~RFrameBufferGL();
    FrameBufferAllocator.Free(&frameBuffer);

    return {};
}

RResult RDeviceGL::CreatePipeline(RPipeline& pipelineH, const RPipelineInfo& info)
{
    RPipelineGL* pipeline = (RPipelineGL*)PipelineAllocator.Alloc(sizeof(RPipelineGL));
    new (pipeline) RPipelineGL{};
    pipeline->Startup(pipelineH, info, *this);

    return {};
}

RResult RDeviceGL::DeletePipeline(RPipeline& pipelineH)
{
    RPipelineGL* pipeline = dynamic_cast<RPipelineGL*>((RPipelineBase*)pipelineH);

    pipeline->Cleanup(pipelineH);
    pipeline->~RPipelineGL();
    PipelineAllocator.Free(pipeline);

    return {};
}

RResult RDeviceGL::GetSwapChainTextureFormat(RTextureFormat& format)
{
    // TODO:
    LD_DEBUG_UNREACHABLE;
    return {};
}

RResult RDeviceGL::GetSwapChainRenderPass(RPass& renderPass)
{
    LD_DEBUG_ASSERT(DefaultRenderPass);

    renderPass = DefaultRenderPass;

    return {};
}

RResult RDeviceGL::GetSwapChainFrameBuffer(RFrameBuffer& frameBuffer)
{
    LD_DEBUG_ASSERT(DefaultFrameBuffer);

    frameBuffer = DefaultFrameBuffer;

    return {};
}

RResult RDeviceGL::BeginFrame()
{
    return {};
}

RResult RDeviceGL::EndFrame()
{
    return {};
}

RResult RDeviceGL::BeginRenderPass(const RPassBeginInfo& info)
{
    RFrameBufferGL& frameBuffer = Derive<RFrameBufferGL>(info.FrameBuffer);
    RPassGL& renderPass = Derive<RPassGL>(info.RenderPass);

    if (frameBuffer.IsDefaultFrameBuffer)
    {
        Context.UnbindFrameBuffer();

        GLenum clearMask = 0;

        // select clear values for default framebuffer attachments
        for (size_t i = 0; i < info.ClearValues.Size(); i++)
        {
            const RClearValue& clearValue = info.ClearValues[i];
            if (clearValue.Color.HasValue())
            {
                RClearColorValue color = clearValue.Color.Value();

                clearMask |= GL_COLOR_BUFFER_BIT;
                glClearColor(color.r, color.g, color.b, color.a);
            }
            if (clearValue.DepthStencil.HasValue())
            {
                RClearDepthStencilValue depthStencil = clearValue.DepthStencil.Value();

                // TODO: query if default framebuffer has depth stencil bits

                clearMask |= GL_DEPTH_BUFFER_BIT;
                glClearDepth(depthStencil.Depth);

                clearMask |= GL_STENCIL_BUFFER_BIT;
                glClearStencil(depthStencil.Stencil);
            }
        }

        // clear the default framebuffer attachments
        if (clearMask != 0)
            glClear(clearMask);

        return {};
    }

    Context.BindFrameBuffer(frameBuffer.FBO);
    frameBuffer.FBO.BindDrawBuffers();

    // clear individual framebuffer attachments
    for (size_t i = 0; i < info.ClearValues.Size(); i++)
    {
        const RClearValue& clearValue = info.ClearValues[i];

        if (clearValue.Color.HasValue())
        {
            const RClearColorValue& colorValue = clearValue.Color.Value();
            glClearBufferfv(GL_COLOR, i, colorValue.Data);
        }
        else if (clearValue.DepthStencil.HasValue())
        {
            if (frameBuffer.FBO.HasDepthBits())
            {
                GLfloat depthValue = (GLfloat)clearValue.DepthStencil.Value().Depth;
                glClearBufferfv(GL_DEPTH, 0, &depthValue);
            }

            if (frameBuffer.FBO.HasStencilBits())
            {
                GLint stencilValue = (GLint)clearValue.DepthStencil.Value().Stencil;
                glClearBufferiv(GL_STENCIL, 0, &stencilValue);
            }
        }
    }

    return {};
}

RResult RDeviceGL::EndRenderPass()
{
    return {};
}

RResult RDeviceGL::SetPipeline(RPipeline& pipelineH)
{
    LD_DEBUG_ASSERT(BoundPipelineH && BoundPipelineH == pipelineH);

    RPipelineGL& pipeline = Derive<RPipelineGL>(BoundPipelineH);
    pipeline.VAO.Bind();
    pipeline.Program.Bind();

    // set rasterization states
    if (pipeline.GLCullMode == GL_NONE)
    {
        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, pipeline.GLPolygonMode);
    }
    else
    {
        GLenum rasterizeFace = pipeline.GLCullMode == GL_BACK ? GL_FRONT : GL_BACK;

        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glCullFace(pipeline.GLCullMode);
        glPolygonMode(rasterizeFace, pipeline.GLPolygonMode);
    }

    // set depth stencil states
    if (pipeline.DepthTestEnabled)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(DeriveGLDepthFunc(pipeline.DepthCompareMode));
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    glDepthMask(pipeline.DepthWriteEnabled);

    // set blend states
    if (pipeline.BlendEnabled)
    {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(pipeline.GLColorBlendSrcFactor, pipeline.GLColorBlendDstFactor,
                            pipeline.GLAlphaBlendSrcFactor, pipeline.GLAlphaBlendDstFactor);
        glBlendEquationSeparate(pipeline.GLColorBlendOp, pipeline.GLAlphaBlendOp);
    }
    else
    {
        glDisable(GL_BLEND);
    }

    return {};
}

RResult RDeviceGL::SetBindingGroup(u32 groupIdx, RBindingGroup& groupH)
{
    LD_DEBUG_ASSERT(BoundPipelineH);

    RBindingGroupGL& group = Derive<RBindingGroupGL>(groupH);
    RPipelineGL& pipeline = Derive<RPipelineGL>(BoundPipelineH);

    // look up table that maps binding qualifiers to OpenGL texture units or buffer bases
    std::unordered_map<u32, u32>& textureUnitBinding = pipeline.TextureUnitBinding[groupIdx];
    std::unordered_map<u32, u32>& uniformBufferBinding = pipeline.UniformBufferBinding[groupIdx];

    // TODO: compare with bound pipeline layout
    pipeline.Program.Bind();

    for (size_t bindingIdx = 0; bindingIdx < group.Bindings.Size(); bindingIdx++)
    {
        RBindingGroupBase::Binding& binding = group.Bindings[bindingIdx];

        switch (binding.Type)
        {
        case RBindingType::Texture:
        {
            int textureUnitBase = (int)textureUnitBinding[bindingIdx];

            for (int arrayIdx = 0; arrayIdx < binding.TextureH.Size(); arrayIdx++)
            {
                if (!binding.TextureH[arrayIdx])
                    continue;

                RTextureGL& texture = Derive<RTextureGL>(binding.TextureH[arrayIdx]);

                // bind OpenGL texture at correct unit
                texture.Bind(textureUnitBase + arrayIdx);
            }
            break;
        }
        case RBindingType::UniformBuffer:
        {
            if (!binding.BufferH)
                break;

            RBufferGL& buffer = Derive<RBufferGL>(binding.BufferH);
            LD_DEBUG_ASSERT(buffer.Target == GL_UNIFORM_BUFFER);

            // bind OpenGL UBO at correct uniform buffer base
            int bufferBase = (int)uniformBufferBinding[bindingIdx];
            buffer.UBO.BindBase(bufferBase);
            break;
        }
        default:
            LD_DEBUG_UNREACHABLE;
        }
    }

    return {};
}

RResult RDeviceGL::SetVertexBuffer(u32 slot, RBuffer& bufferH)
{
    RPipelineGL& pipeline = Derive<RPipelineGL>(BoundPipelineH);
    RBufferGL& buffer = Derive<RBufferGL>(bufferH);

    LD_DEBUG_ASSERT(buffer.Target == GL_ARRAY_BUFFER && "binding a non vertex buffer");

    pipeline.VAO.Bind();

    glBindVertexBuffer(slot, (GLuint)buffer.VBO, 0, pipeline.VertexStrides[slot]);

    return {};
}

RResult RDeviceGL::SetIndexBuffer(RBuffer& bufferH, RIndexType indexType)
{
    RBufferGL& buffer = Derive<RBufferGL>(bufferH);

    LD_DEBUG_ASSERT(buffer.Target == GL_ELEMENT_ARRAY_BUFFER && "binding a non index buffer");

    buffer.Bind();
    IndexType = DeriveGLIndexType(indexType);

    return {};
}

RResult RDeviceGL::PushScissor(const Rect2D& scissor)
{
    if (Scissors.IsEmpty())
    {
        glEnable(GL_SCISSOR_TEST);
    }

    Scissors.Push(scissor);

    GLint sx = scissor.x;
    GLint sy = ViewportExtent.y - scissor.h - scissor.y;
    glScissor(sx, sy, (GLsizei)scissor.w, (GLsizei)scissor.h);

    return {};
}

RResult RDeviceGL::PopScissor()
{
    LD_DEBUG_ASSERT(!Scissors.IsEmpty());

    Scissors.Pop();

    if (Scissors.IsEmpty())
    {
        glDisable(GL_SCISSOR_TEST);
    }
    else
    {
        const Rect2D& scissor = Scissors.Top();
        GLint sx = scissor.x;
        GLint sy = ViewportExtent.y - scissor.h - scissor.y;
        glScissor(sx, sy, (GLsizei)scissor.w, (GLsizei)scissor.h);
    }

    return {};
}

RResult RDeviceGL::DrawVertex(const RDrawVertexInfo& info)
{
    LD_DEBUG_ASSERT(BoundPipelineH);
    LD_DEBUG_ASSERT(info.VertexStart == 0);

    RPipelineGL& pipeline = Derive<RPipelineGL>(BoundPipelineH);

    GLCommand::DrawArraysInstanced(pipeline.GLPrimitiveTopology, info.VertexCount, info.InstanceCount,
                                   info.InstanceStart);

    return {};
}

RResult RDeviceGL::DrawIndexed(const RDrawIndexedInfo& info)
{
    LD_DEBUG_ASSERT(BoundPipelineH);

    RPipelineGL& pipeline = Derive<RPipelineGL>(BoundPipelineH);

    GLCommand::DrawElementsInstanced(pipeline.GLPrimitiveTopology, info.IndexCount, IndexType, info.InstanceCount,
                                     info.IndexStart, info.InstanceStart);

    return {};
}

RResult RDeviceGL::ResizeViewport(int width, int height)
{
    ViewportExtent.x = width;
    ViewportExtent.y = height;
    glViewport(0, 0, width, height);

    return {};
}

RResult RDeviceGL::CreateDefaultFrameBuffer(RFrameBuffer& frameBufferH)
{
    RFrameBufferGL* frameBuffer = (RFrameBufferGL*)FrameBufferAllocator.Alloc(sizeof(RFrameBufferGL));
    new (frameBuffer) RFrameBufferGL{};

    frameBuffer->IsDefaultFrameBuffer = true;
    frameBuffer->Startup(frameBufferH, {}, *this);

    return {};
}

RResult RDeviceGL::DeleteDefaultFrameBuffer(RFrameBuffer& frameBufferH)
{
    RFrameBufferGL& frameBuffer = Derive<RFrameBufferGL>(frameBufferH);

    LD_DEBUG_ASSERT(frameBuffer.IsDefaultFrameBuffer);
    frameBuffer.Cleanup(frameBufferH);

    frameBuffer.~RFrameBufferGL();
    FrameBufferAllocator.Free(&frameBuffer);

    return {};
}

} // namespace LD
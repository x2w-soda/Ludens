#include "Core/RenderBase/Lib/RPipelineGL.h"
#include "Core/RenderBase/Lib/RShaderGL.h"
#include "Core/RenderBase/Lib/RDeriveGL.h"
#include "Core/RenderBase/Lib/RDeviceGL.h"

namespace LD
{

RPipelineGL::RPipelineGL()
{
}

RPipelineGL::~RPipelineGL()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RPipelineGL::Startup(RPipeline& pipelineH, const RPipelineInfo& info, RDeviceGL& device)
{
    RPipelineBase::Startup(pipelineH, info, &device);

    GLPrimitiveTopology = DeriveGLPrimitiveTopology(info.PrimitiveTopology);
    GLPolygonMode =
        PolygonMode == RPolygonMode::Fill ? GL_FILL : (PolygonMode == RPolygonMode::Line ? GL_LINE : GL_POINT);
    GLCullMode = CullMode == RCullMode::BackFace ? GL_BACK : (CullMode == RCullMode::FrontFace ? GL_FRONT : GL_NONE);

    BlendEnabled = info.BlendState.BlendEnabled;
    if (BlendEnabled)
    {
        GLColorBlendSrcFactor = DeriveGLBlendFactor(info.BlendState.ColorSrcFactor);
        GLColorBlendDstFactor = DeriveGLBlendFactor(info.BlendState.ColorDstFactor);
        GLColorBlendOp = DeriveGLBlendOp(info.BlendState.ColorBlendMode);
        GLAlphaBlendSrcFactor = DeriveGLBlendFactor(info.BlendState.AlphaSrcFactor);
        GLAlphaBlendDstFactor = DeriveGLBlendFactor(info.BlendState.AlphaDstFactor);
        GLAlphaBlendOp = DeriveGLBlendOp(info.BlendState.AlphaBlendMode);
    }

    RShaderGL& vertexShader = Derive<RShaderGL>(VertexShaderH);
    RShaderGL& fragmentShader = Derive<RShaderGL>(FragmentShaderH);
    size_t groupCount = GroupLayoutsH.Size();

    // This is based on how RShaderCompiler patches GLSL for OpenGL:
    // - texture binding with lower qualifier uses lower texture unit slot
    // - uniform buffer with lower qualifier uses lower buffer base
    {
        TextureUnitBinding.Resize(groupCount);
        UniformBufferBinding.Resize(groupCount);

        int textureUnit = 0;
        int uniformBufferBase = 0;

        for (size_t groupIdx = 0; groupIdx < groupCount; groupIdx++)
        {
            RBindingGroupLayoutGL& groupLayout = Derive<RBindingGroupLayoutGL>(GroupLayoutsH[groupIdx]);

            for (size_t bindingIdx = 0; bindingIdx < groupLayout.Bindings.Size(); bindingIdx++)
            {
                const RBindingInfo& binding = groupLayout.Bindings[bindingIdx];

                switch (binding.Type)
                {
                case RBindingType::Texture:
                    TextureUnitBinding[groupIdx][bindingIdx] = textureUnit;
                    textureUnit += binding.Count;
                    break;
                case RBindingType::UniformBuffer:
                    UniformBufferBinding[groupIdx][bindingIdx] = uniformBufferBase++;
                    break;
                }
            }
        }
    }

    GLProgramInfo programInfo{};
    programInfo.IsSpirvData = vertexShader.SourceType == RShaderSourceType::SPIRV;
    programInfo.VertexShaderData = vertexShader.Source.data();
    programInfo.VertexShaderSize = vertexShader.Source.size();
    programInfo.FragmentShaderData = fragmentShader.Source.data();
    programInfo.FragmentShaderSize = fragmentShader.Source.size();
    Program.Startup(device.Context, programInfo);

    // One VAO per pipeline is used to capture how vertex attributes are polled from one or more vertex buffers.
    VAO.Startup(device.Context);
    VAO.Bind();

    const RVertexLayout& vertexLayout = info.VertexLayout;
    u32 vertexBufferSlotCount = vertexLayout.Slots.Size();
    VertexStrides.Resize(vertexBufferSlotCount);

    for (size_t slotIdx = 0; slotIdx < vertexBufferSlotCount; slotIdx++)
    {
        const RVertexBufferSlot& slot = vertexLayout.Slots[slotIdx];

        GLVertexLayout layout;
        DeriveGLVertexLayout(slot, layout);
        VertexStrides[slotIdx] = layout.GetVertexStride();
        const Vector<GLVertexAttribute>& attrs = layout.GetAttributes();
        const Vector<u32>& offsets = layout.GetOffsets();

        if (slot.PollRate == RAttributePollRate::PerInstance)
        {
            // this vertex buffer slot contains per-instance attributes
            glVertexBindingDivisor(slotIdx, 1);
        }

        for (size_t attrIdx = 0; attrIdx < attrs.Size(); attrIdx++)
        {
            const GLVertexAttribute& attr = attrs[attrIdx];
            GLint attrComponentCount;
            GLenum attrComponentType;
            u32 attrByteSize;
            GetGLSLTypeVertexAttribute(attr.Type, &attrComponentCount, &attrComponentType, &attrByteSize);

            glEnableVertexAttribArray(attr.Location);
            glVertexAttribFormat(attr.Location, attrComponentCount, attrComponentType, attr.Normalize,
                                 offsets[attrIdx]);
            glVertexAttribBinding(attr.Location, slotIdx);
        }
    }
}

void RPipelineGL::Cleanup(RPipeline& pipelineH)
{
    RPipelineBase::Cleanup(pipelineH);

    VAO.Cleanup();
    Program.Cleanup();
}

} // namespace LD
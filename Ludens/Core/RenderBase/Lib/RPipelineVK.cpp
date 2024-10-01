#include "Core/RenderBase/Include/VK/VKDescriptor.h"
#include "Core/RenderBase/Include/VK/VKInfo.h"
#include "Core/RenderBase/Lib/RPipelineVK.h"
#include "Core/RenderBase/Lib/RDeriveVK.h"
#include "Core/RenderBase/Lib/RBindingVK.h"
#include "Core/RenderBase/Lib/RShaderVK.h"
#include "Core/RenderBase/Lib/RDeviceVK.h"

namespace LD
{

RPipelineVK::RPipelineVK()
{
}

RPipelineVK::~RPipelineVK()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RPipelineVK::Startup(RPipeline& pipelineH, const RPipelineInfo& info, RDeviceVK& device)
{
    RPipelineBase::Startup(pipelineH, info, (RDeviceBase*)&device);
    VKContext& vkContext = device.Context;
    VKRenderPass& vkRenderPass = Derive<RPassVK>(info.RenderPass).RenderPass;
    VkExtent2D vkSwapChainExtent = vkContext.GetSwapChain().GetExtent();

    VKVertexLayout vertexLayout;
    DeriveVKVertexLayout(info.VertexLayout, vertexLayout);

    VKShader& vertexShader = Derive<RShaderVK>(info.VertexShader).ShaderModule;
    VKShader& fragmentShader = Derive<RShaderVK>(info.FragmentShader).ShaderModule;
    VkPrimitiveTopology topology = DeriveVKPrimitiveTopology(info.PrimitiveTopology);

    // each binding group layout corresponds to a vulkan descriptor set layout
    const RPipelineLayout& pipelineLayout = info.PipelineLayout;
    size_t groupCount = pipelineLayout.GroupLayouts.Size();

    Vector<VkDescriptorSetLayout> setLayouts(groupCount);
    for (size_t groupIdx = 0; groupIdx < groupCount; groupIdx++)
    {
        RBindingGroupLayoutVK& groupLayout = Derive<RBindingGroupLayoutVK>(pipelineLayout.GroupLayouts[groupIdx]);

        setLayouts[groupIdx] = groupLayout.DescriptorSetLayout.GetHandle();
    }

    PipelineLayout.SetPushConstantRanges(0, nullptr)
        .SetDescriptorSetLayouts(setLayouts.Size(), setLayouts.Data())
        .Startup(vkContext.GetDevice());

    VkViewport viewport = VKInfo::Viewport(vkSwapChainExtent);
    VkRect2D scissor = VKInfo::Rect2D(vkSwapChainExtent);
    VkPolygonMode polygonMode =
        PolygonMode == RPolygonMode::Fill
            ? VK_POLYGON_MODE_FILL
            : (PolygonMode == RPolygonMode::Line ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_POINT);
    VkCullModeFlags cullMode = CullMode == RCullMode::BackFace
                                   ? VK_CULL_MODE_BACK_BIT
                                   : (CullMode == RCullMode::FrontFace ? VK_CULL_MODE_FRONT_BIT : VK_CULL_MODE_NONE);

    VkPipelineRasterizationStateCreateInfo rasterizerStateCI{};
    rasterizerStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerStateCI.depthClampEnable = VK_FALSE;
    rasterizerStateCI.rasterizerDiscardEnable = VK_FALSE;
    rasterizerStateCI.polygonMode = polygonMode;
    rasterizerStateCI.lineWidth = 1.0f;
    rasterizerStateCI.cullMode = cullMode;
    rasterizerStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizerStateCI.depthBiasEnable = VK_FALSE;
    rasterizerStateCI.depthBiasConstantFactor = 0.0f;
    rasterizerStateCI.depthBiasClamp = 0.0f;
    rasterizerStateCI.depthBiasSlopeFactor = 0.0f;

    VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
    depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCI.depthTestEnable = DepthTestEnabled ? VK_TRUE : VK_FALSE;
    depthStencilStateCI.depthWriteEnable = DepthWriteEnabled ? VK_TRUE : VK_FALSE;
    depthStencilStateCI.depthCompareOp = DeriveVKCompareOp(DepthCompareMode);
    depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCI.stencilTestEnable = VK_FALSE;

    // NOTE: currently all pipeline color attachments use the same blend state.
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = info.BlendState.BlendEnabled;
    if (colorBlendAttachment.blendEnable)
    {
        colorBlendAttachment.srcColorBlendFactor = DeriveVKBlendFactor(info.BlendState.ColorSrcFactor);
        colorBlendAttachment.dstColorBlendFactor = DeriveVKBlendFactor(info.BlendState.ColorDstFactor);
        colorBlendAttachment.colorBlendOp = DeriveVKBlendOp(info.BlendState.ColorBlendMode);
        colorBlendAttachment.srcAlphaBlendFactor = DeriveVKBlendFactor(info.BlendState.AlphaSrcFactor);
        colorBlendAttachment.dstAlphaBlendFactor = DeriveVKBlendFactor(info.BlendState.AlphaDstFactor);
        colorBlendAttachment.alphaBlendOp = DeriveVKBlendOp(info.BlendState.AlphaBlendMode);
    }

    Pipeline.SetName(info.Name)
        .SetViewports({ viewport })
        .SetScissors({ scissor })
        .SetInputAssembly(topology)
        .SetVertexShaderStage(vertexShader)
        .SetFragmentShaderStage(fragmentShader)
        .SetVertexLayout(vertexLayout)
        .SetPipelineLayout(PipelineLayout)
        .SetRasterizationState(rasterizerStateCI)
        .SetDepthStencilState(depthStencilStateCI)
        .SetBlendState(colorBlendAttachment)
        .SetRenderPass(vkRenderPass, 0)
        .Startup(vkContext.GetDevice());
}

void RPipelineVK::Cleanup(RPipeline& pipelineH)
{
    RPipelineBase::Cleanup(pipelineH);

    PipelineLayout.Cleanup();
    Pipeline.Cleanup();
}

} // namespace LD
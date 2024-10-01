#include <iostream>
#include "Core/Header/Include/Error.h"
#include "Core/OS/Include/Memory.h"
#include "Core/RenderBase/Include/VK/VKInfo.h"
#include "Core/RenderBase/Include/VK/VKPipeline.h"
#include "Core/RenderBase/Include/VK/VKRenderPass.h"
#include "Core/RenderBase/Include/VK/VKShader.h"
#include "Core/RenderBase/Include/VK/VKContext.h"

namespace LD
{

VKPipelineLayout::VKPipelineLayout()
{
}

VKPipelineLayout::~VKPipelineLayout()
{
    LD_DEBUG_ASSERT(mHandle == VK_NULL_HANDLE);
}

VKPipelineLayout& VKPipelineLayout::SetPushConstantRanges(u32 count, const VkPushConstantRange* ranges)
{
    mRangeCount = count;
    mRanges = ranges;

    return *this;
}

VKPipelineLayout& VKPipelineLayout::SetDescriptorSetLayouts(u32 count, const VkDescriptorSetLayout* setLayouts)
{
    mSetLayoutCount = count;
    mSetLayouts = setLayouts;

    return *this;
}

void VKPipelineLayout::Startup(const VKDevice& device)
{
    mDevice = device.GetHandle();

    VkPipelineLayoutCreateInfo layoutCI{};
    layoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCI.setLayoutCount = mSetLayoutCount;
    layoutCI.pSetLayouts = mSetLayouts;
    layoutCI.pushConstantRangeCount = mRangeCount;
    layoutCI.pPushConstantRanges = mRanges;

    VK_ASSERT(vkCreatePipelineLayout(mDevice, &layoutCI, nullptr, &mHandle));
}

void VKPipelineLayout::Cleanup()
{
    vkDestroyPipelineLayout(mDevice, mHandle, nullptr);

    mHandle = VK_NULL_HANDLE;
}

VKPipeline::VKPipeline()
{
}

VKPipeline::~VKPipeline()
{
    LD_DEBUG_ASSERT(mHandle == VK_NULL_HANDLE);
}

VKPipeline& VKPipeline::SetName(const char* pipelineName)
{
    mPipelineName = pipelineName;

    return *this;
}

VKPipeline& VKPipeline::SetViewports(const Vector<VkViewport>& viewports)
{
    mViewports = viewports;

    return *this;
}

VKPipeline& VKPipeline::SetScissors(const Vector<VkRect2D>& scissors)
{
    mScissors = scissors;

    return *this;
}

VKPipeline& VKPipeline::SetInputAssembly(const VkPrimitiveTopology& topology)
{
    mPrimitiveTopology = topology;

    return *this;
}

VKPipeline& VKPipeline::SetVertexShaderStage(const VKShader& shader)
{
    LD_DEBUG_ASSERT(!mHasVertexShader && shader.GetStage() == VK_SHADER_STAGE_VERTEX_BIT);

    VkPipelineShaderStageCreateInfo info = VKInfo::PipelineShaderStageCreateVertex(shader.GetHandle());
    mShaderStages.PushBack(info);
    mHasVertexShader = true;

    return *this;
}

VKPipeline& VKPipeline::SetFragmentShaderStage(const VKShader& shader)
{
    LD_DEBUG_ASSERT(!mHasFragmentShader && shader.GetStage() == VK_SHADER_STAGE_FRAGMENT_BIT);

    VkPipelineShaderStageCreateInfo info = VKInfo::PipelineShaderStageCreateFragment(shader.GetHandle());
    mShaderStages.PushBack(info);
    mHasFragmentShader = true;

    return *this;
}

VKPipeline& VKPipeline::SetVertexLayout(const VKVertexLayout& vertexLayout)
{
    mVertexBindings = vertexLayout.GetBindings();
    mVertexAttributes = vertexLayout.GetAttributes();

    return *this;
}

VKPipeline& VKPipeline::SetPipelineLayout(const VKPipelineLayout& pipelineLayout)
{
    mPipelineLayout = pipelineLayout.GetHandle();

    return *this;
}

VKPipeline& VKPipeline::SetRasterizationState(const VkPipelineRasterizationStateCreateInfo& rasterizationState)
{
    mRasterizationState = rasterizationState;

    return *this;
}

VKPipeline& VKPipeline::SetDepthStencilState(const VkPipelineDepthStencilStateCreateInfo& depthStencilState)
{
    mDepthStencilState = depthStencilState;

    return *this;
}

VKPipeline& VKPipeline::SetBlendState(const VkPipelineColorBlendAttachmentState& colorBlendState)
{
    mColorBlendState = colorBlendState;

    return *this;
}

VKPipeline& VKPipeline::SetRenderPass(const VKRenderPass& renderPass, int subPass)
{
    mRenderPass = renderPass.GetHandle();
    mColorAttachmentCount = renderPass.GetColorAttachmentCount();
    mSubPass = subPass;

    return *this;
}

void VKPipeline::Startup(const VKDevice& device)
{
    LD_DEBUG_ASSERT(!mShaderStages.IsEmpty());
    LD_DEBUG_ASSERT(!mViewports.IsEmpty());
    LD_DEBUG_ASSERT(!mScissors.IsEmpty());
    LD_DEBUG_ASSERT(mPipelineLayout != VK_NULL_HANDLE);
    LD_DEBUG_ASSERT(mRenderPass != VK_NULL_HANDLE);
    LD_DEBUG_ASSERT(mHasVertexShader && mHasFragmentShader);

    mDevice = device.GetHandle();

    // PIPELINE DYNAMIC STATES
    // all pipelines can dynamically change viewport and scissor
    // without having to recreate a new pipeline.
    Vector<VkDynamicState> dynamicStates{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamicStateCI{};
    dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCI.dynamicStateCount = dynamicStates.Size();
    dynamicStateCI.pDynamicStates = dynamicStates.Data();

    // PIPELINE VERTEX INPUT
    VkPipelineVertexInputStateCreateInfo vertexInputStateCI = VKInfo::PipelineVertexInputStateCreate(
        mVertexBindings.Size(), mVertexBindings.Data(), mVertexAttributes.Size(), mVertexAttributes.Data());

    // PIPELINE INPUT ASSEMBLY
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI =
        VKInfo::PipelineInputAssemblyStateCreate(mPrimitiveTopology, VK_FALSE);

    // PIPELINE VIEWPORT
    VkPipelineViewportStateCreateInfo viewportStateCI =
        VKInfo::PipelineViewportStateCreate(mViewports.Size(), mViewports.Data(), mScissors.Size(), mScissors.Data());

    // PIPELINE MULTI-SAMPLING
    VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
    multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCI.sampleShadingEnable = VK_FALSE;
    multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateCI.minSampleShading = 1.0f;
    multisampleStateCI.pSampleMask = nullptr;
    multisampleStateCI.alphaToCoverageEnable = VK_FALSE;
    multisampleStateCI.alphaToOneEnable = VK_FALSE;

    // PIPELINE COLOR BLEND
    Vector<VkPipelineColorBlendAttachmentState> colorBlendAttachment(mColorAttachmentCount);
    if (!colorBlendAttachment.IsEmpty())
    {
        std::fill(colorBlendAttachment.Begin(), colorBlendAttachment.End(), mColorBlendState);
    }

    VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
    colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCI.logicOpEnable = VK_FALSE;
    colorBlendStateCI.logicOp = VK_LOGIC_OP_COPY;
    colorBlendStateCI.attachmentCount = colorBlendAttachment.Size();
    colorBlendStateCI.pAttachments = colorBlendAttachment.Data();
    colorBlendStateCI.blendConstants[0] = 0.0f;
    colorBlendStateCI.blendConstants[1] = 0.0f;
    colorBlendStateCI.blendConstants[2] = 0.0f;
    colorBlendStateCI.blendConstants[3] = 0.0f;

    VkGraphicsPipelineCreateInfo pipelineCI{};
    pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCI.stageCount = mShaderStages.Size();
    pipelineCI.pStages = mShaderStages.Data();
    pipelineCI.pDynamicState = &dynamicStateCI;
    pipelineCI.pVertexInputState = &vertexInputStateCI;
    pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
    pipelineCI.pViewportState = &viewportStateCI;
    pipelineCI.pRasterizationState = &mRasterizationState;
    pipelineCI.pMultisampleState = &multisampleStateCI;
    pipelineCI.pDepthStencilState = &mDepthStencilState;
    pipelineCI.pColorBlendState = &colorBlendStateCI;
    pipelineCI.layout = mPipelineLayout;
    pipelineCI.renderPass = mRenderPass;
    pipelineCI.subpass = mSubPass;

    VK_ASSERT(vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &mHandle));
}

void VKPipeline::Cleanup()
{
    vkDestroyPipeline(mDevice, mHandle, nullptr);

    mHandle = VK_NULL_HANDLE;
}

} // namespace LD
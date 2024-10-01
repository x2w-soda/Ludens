#pragma once

#include <string>
#include <vulkan/vulkan_core.h>
#include "Core/Header/Include/Types.h"
#include "Core/DSA/Include/Vector.h"
#include "Core/RenderBase/Include/VK/VKVertex.h"

namespace LD
{

class VKShader;
class VKRenderPass;
class VKDevice;

class VKPipelineLayout
{
public:
    VKPipelineLayout();
    VKPipelineLayout(const VKPipelineLayout&) = delete;
    ~VKPipelineLayout();

    VKPipelineLayout& operator=(const VKPipelineLayout&) = delete;

    VKPipelineLayout& SetPushConstantRanges(u32 count, const VkPushConstantRange* ranges);
    VKPipelineLayout& SetDescriptorSetLayouts(u32 count, const VkDescriptorSetLayout* setLayouts);
    void Startup(const VKDevice& device);
    void Cleanup();

    inline VkPipelineLayout GetHandle() const
    {
        return mHandle;
    }

private:
    VkPipelineLayout mHandle = VK_NULL_HANDLE;
    VkDevice mDevice = VK_NULL_HANDLE;
    const VkPushConstantRange* mRanges = nullptr;
    const VkDescriptorSetLayout* mSetLayouts = nullptr;
    u32 mRangeCount = 0;
    u32 mSetLayoutCount = 0;
};

class VKPipeline
{
public:
    VKPipeline();
    VKPipeline(const VKPipeline&) = delete;
    ~VKPipeline();

    VKPipeline& operator=(const VKPipeline&) = delete;

    VKPipeline& SetName(const char* pipelineName);
    VKPipeline& SetViewports(const Vector<VkViewport>& viewports);
    VKPipeline& SetScissors(const Vector<VkRect2D>& scissors);
    VKPipeline& SetInputAssembly(const VkPrimitiveTopology& topology);
    VKPipeline& SetVertexShaderStage(const VKShader& shader);
    VKPipeline& SetFragmentShaderStage(const VKShader& shader);
    VKPipeline& SetVertexLayout(const VKVertexLayout& vertexLayout);
    VKPipeline& SetPipelineLayout(const VKPipelineLayout& pipelineLayout);
    VKPipeline& SetRenderPass(const VKRenderPass& renderPass, int subPass);
    VKPipeline& SetRasterizationState(const VkPipelineRasterizationStateCreateInfo& rasterizationState);
    VKPipeline& SetDepthStencilState(const VkPipelineDepthStencilStateCreateInfo& depthStencilState);
    VKPipeline& SetBlendState(const VkPipelineColorBlendAttachmentState& colorBlendState);
    void Startup(const VKDevice& device);
    void Cleanup();

    inline VkPipeline GetHandle() const
    {
        return mHandle;
    }

private:
    VkPipeline mHandle = VK_NULL_HANDLE;
    VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
    VkRenderPass mRenderPass = VK_NULL_HANDLE;
    VkDevice mDevice = VK_NULL_HANDLE;
    VKVertexBindings mVertexBindings;
    VKVertexAttributes mVertexAttributes;
    VkPrimitiveTopology mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    int mSubPass;
    int mColorAttachmentCount;
    std::string mPipelineName;
    Vector<VkViewport> mViewports;
    Vector<VkRect2D> mScissors;
    Vector<VkPipelineShaderStageCreateInfo> mShaderStages;
    VkPipelineRasterizationStateCreateInfo mRasterizationState;
    VkPipelineDepthStencilStateCreateInfo mDepthStencilState;
    VkPipelineColorBlendAttachmentState mColorBlendState;
    bool mHasVertexShader = false;
    bool mHasFragmentShader = false;
    bool mHasStartup = false;
};

} // namespace LD
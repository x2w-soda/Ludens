#include <iostream>
#include "Core/RenderBase/Include/VK/VKContext.h"
#include "Core/RenderBase/Include/VK/VKDevice.h"
#include "Core/RenderBase/Include/VK/VKRenderPass.h"
#include "Core/RenderBase/Include/VK/VKSwapChain.h"
#include "Core/RenderBase/Include/VK/VKFormat.h"

namespace LD
{

VKRenderPass::~VKRenderPass()
{
    LD_DEBUG_ASSERT(mHandle == VK_NULL_HANDLE);
    LD_DEBUG_ASSERT(mDevice == VK_NULL_HANDLE);
}

VKRenderPass& VKRenderPass::SetAttachments(const Attachments& attachments)
{
    mAttachments = attachments;
    return *this;
}

VKRenderPass& VKRenderPass::SetSubPasses(const SubPasses& subPasses)
{
    mSubPasses = subPasses;
    return *this;
}

VKRenderPass& VKRenderPass::SetSubPassDependencies(const SubPassDependencies& dependencies)
{
    mSubPassDependencies = dependencies;
    return *this;
}

void VKRenderPass::Startup(const VKContext& context)
{
    mDevice = context.GetDevice().GetHandle();

    VkRenderPassCreateInfo renderPassCI{};
    renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCI.attachmentCount = mAttachments.Size();
    renderPassCI.pAttachments = mAttachments.Data();
    renderPassCI.subpassCount = mSubPasses.Size();
    renderPassCI.pSubpasses = mSubPasses.Data();
    renderPassCI.dependencyCount = mSubPassDependencies.Size();
    renderPassCI.pDependencies = mSubPassDependencies.Data();

    VK_ASSERT(vkCreateRenderPass(mDevice, &renderPassCI, nullptr, &mHandle));

    std::cout << "VKRenderPass setup complete" << std::endl;

    mHasStartup = true;
}

void VKRenderPass::Cleanup()
{
    mHasStartup = false;

    vkDestroyRenderPass(mDevice, mHandle, nullptr);
    mDevice = VK_NULL_HANDLE;
    mHandle = VK_NULL_HANDLE;

    std::cout << "VKRenderPass cleanup complete" << std::endl;
}

size_t VKRenderPass::GetColorAttachmentCount() const
{
    LD_DEBUG_ASSERT(mHandle != VK_NULL_HANDLE);

    size_t count = 0;

    for (const VkAttachmentDescription& attachment : mAttachments)
    {
        if (!VKFormat::HasDepthComponent(attachment.format) && !VKFormat::HasStencilComponent(attachment.format))
            count++;
    }

    return count;
}

} // namespace LD
#pragma once

#include <vulkan/vulkan_core.h>
#include "Core/DSA/Include/Vector.h"

namespace LD
{

class VKContext;

class VKRenderPass
{
public:
    using Attachments = Vector<VkAttachmentDescription>;
    using SubPasses = Vector<VkSubpassDescription>;
    using SubPassDependencies = Vector<VkSubpassDependency>;

    VKRenderPass() = default;
    VKRenderPass(const VKRenderPass&) = delete;
    ~VKRenderPass();

    VKRenderPass& operator=(const VKRenderPass&) = delete;

    VKRenderPass& SetAttachments(const Attachments& attachments);
    VKRenderPass& SetSubPasses(const SubPasses& subPasses);
    VKRenderPass& SetSubPassDependencies(const SubPassDependencies& dependencies);
    void Startup(const VKContext& context);
    void Cleanup();

    inline bool IsValid() const
    {
        return mHandle != VK_NULL_HANDLE && mHasStartup;
    }

    inline VkRenderPass GetHandle() const
    {
        return mHandle;
    }

    size_t GetColorAttachmentCount() const;

private:
    bool mHasStartup = false;
    Attachments mAttachments;
    SubPasses mSubPasses;
    SubPassDependencies mSubPassDependencies;
    VkDevice mDevice = VK_NULL_HANDLE;
    VkRenderPass mHandle = VK_NULL_HANDLE;
};

} // namespace LD
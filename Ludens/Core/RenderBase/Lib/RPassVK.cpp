#include "Core/RenderBase/Include/VK/VKInfo.h"
#include "Core/RenderBase/Lib/RDeviceVK.h"
#include "Core/RenderBase/Lib/RDeriveVK.h"
#include "Core/RenderBase/Lib/RPassVK.h"

namespace LD
{

RPassVK::RPassVK()
{
}

RPassVK::~RPassVK()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RPassVK::Startup(RPass& passH, const RPassInfo& info, RDeviceVK& device)
{
    RPassBase::Startup(passH, info, (RDeviceBase*)&device);

    Vector<VkAttachmentReference> colorAttachmentRefs;
    Optional<VkAttachmentReference> depthStencilAttachmentRef;

    Vector<VkAttachmentDescription> attachments(info.Attachments.Size());
    for (size_t i = 0; i < attachments.Size(); i++)
    {
        DeriveVKAttachmentDescription(info.Attachments[i], attachments[i]);

        // TODO: parameterize subpass image layout state
        if (VKFormat::HasDepthComponent(attachments[i].format) || VKFormat::HasStencilComponent(attachments[i].format))
        {
            LD_DEBUG_ASSERT(!depthStencilAttachmentRef.HasValue() && "multiple depth stencil attachments");
            depthStencilAttachmentRef = VKInfo::AttachmentRef(i, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        }
        else
        {
            colorAttachmentRefs.PushBack(VKInfo::AttachmentRef(i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
        }
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = colorAttachmentRefs.Size();
    subpass.pColorAttachments = colorAttachmentRefs.Data();
    subpass.inputAttachmentCount = 0;
    subpass.pDepthStencilAttachment = depthStencilAttachmentRef.HasValue() ? depthStencilAttachmentRef.Data() : nullptr;

    RenderPass.SetAttachments(attachments).SetSubPasses({ subpass }).Startup(device.Context);
}

void RPassVK::Cleanup(RPass& passH)
{
    RPassBase::Cleanup(passH);

    RenderPass.Cleanup();
}

} // namespace LD
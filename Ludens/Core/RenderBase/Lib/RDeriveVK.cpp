#include "Core/RenderBase/Include/VK/VKInfo.h"
#include "Core/RenderBase/Lib/RDeriveVK.h"
#include "Core/RenderBase/Lib/RBindingVK.h"

namespace LD
{

struct TextureFormatMap
{
    RTextureFormat RFormat;
    VkFormat VKFormat;
};

// clang-format off
static const TextureFormatMap sTextureFormatMap[]{
    { RTextureFormat::Undefined,   VK_FORMAT_UNDEFINED },
    { RTextureFormat::R8,          VK_FORMAT_R8_UNORM },
    { RTextureFormat::BGRA8,       VK_FORMAT_B8G8R8A8_UNORM },
    { RTextureFormat::RGBA8,       VK_FORMAT_R8G8B8A8_UNORM },
    { RTextureFormat::RGBA16F,     VK_FORMAT_R16G16B16A16_SFLOAT },
    { RTextureFormat::D24S8,       VK_FORMAT_D24_UNORM_S8_UINT },
    { RTextureFormat::D32F,        VK_FORMAT_D32_SFLOAT },
};
// clang-format on

LD_STATIC_ASSERT(sizeof(sTextureFormatMap) / sizeof(*sTextureFormatMap) == (size_t)RTextureFormat::EnumCount);

static VkFormat DeriveVKVertexAttributeFormat(RDataType type, u32* size)
{
    VkFormat attrFormat;
    u32 attrSize;

    switch (type)
    {
    case RDataType::Float:
        attrFormat = VK_FORMAT_R32_SFLOAT;
        attrSize = 4;
        break;
    case RDataType::Vec2:
        attrFormat = VK_FORMAT_R32G32_SFLOAT;
        attrSize = 8;
        break;
    case RDataType::Vec3:
        attrFormat = VK_FORMAT_R32G32B32_SFLOAT;
        attrSize = 12;
        break;
    case RDataType::Vec4:
        attrFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
        attrSize = 16;
        break;
    default:
        LD_DEBUG_UNREACHABLE;
    }

    if (size)
        *size = attrSize;

    return attrFormat;
}

static VkImageLayout DeriveVKImageLayout(RState state)
{
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

    switch (state)
    {
    case RState::ColorAttachment:
        layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        break;
    case RState::Present:
        layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        break;
    case RState::DepthStencilRead:
        layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        break;
    case RState::DepthStencilWrite:
        layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        break;
    case RState::ShaderResource:
        layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        break;
    default:
        break;
    }

    return layout;
}

static VkAttachmentLoadOp DeriveVKAttachmentLoadOp(RLoadOp loadOp)
{
    VkAttachmentLoadOp vkLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

    switch (loadOp)
    {
    case RLoadOp::Load:
        vkLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        break;
    case RLoadOp::Clear:
        vkLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        break;
    case RLoadOp::Discard:
    default:
        break;
    }

    return vkLoadOp;
}

static VkAttachmentStoreOp DeriveVKAttachmentStoreOp(RStoreOp storeOp)
{
    VkAttachmentStoreOp vkStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    switch (storeOp)
    {
    case RStoreOp::Store:
        vkStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        break;
    case RStoreOp::Discard:
    default:
        break;
    }

    return vkStoreOp;
}

void DeriveVKSamplerAddressMode(const RSamplerAddressMode& inAddrMode, VkSamplerAddressMode& outAddrMode)
{
    switch (inAddrMode)
    {
    case RSamplerAddressMode::Repeat:
        outAddrMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        break;
    case RSamplerAddressMode::ClampToEdge:
        outAddrMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        break;
    default:
        LD_DEBUG_UNREACHABLE;
    }
}

void DeriveVKSamplerFilter(const RSamplerFilter& inFilter, VkFilter& outFilter)
{
    switch (inFilter)
    {
    case RSamplerFilter::Linear:
        outFilter = VK_FILTER_LINEAR;
        break;
    case RSamplerFilter::Nearest:
        outFilter = VK_FILTER_NEAREST;
        break;
    default:
        LD_DEBUG_UNREACHABLE;
    }
}

void DeriveVKAttachmentDescription(const RPassAttachment& inAttachment, VkAttachmentDescription& outAttachment)
{
    // TODO: care about the stencil components

    outAttachment.format = DeriveVKImageFormat(inAttachment.Format);
    outAttachment.initialLayout = DeriveVKImageLayout(inAttachment.InitialState);
    outAttachment.finalLayout = DeriveVKImageLayout(inAttachment.FinalState);
    outAttachment.loadOp = DeriveVKAttachmentLoadOp(inAttachment.LoadOp);
    outAttachment.storeOp = DeriveVKAttachmentStoreOp(inAttachment.StoreOp);
    outAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    outAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    outAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
}

void DeriveVKVertexLayout(const RVertexLayout& inLayout, VKVertexLayout& outLayout)
{
    for (size_t slotIdx = 0; slotIdx < inLayout.Slots.Size(); slotIdx++)
    {
        const RVertexBufferSlot& slot = inLayout.Slots[slotIdx];
        u32 stride = 0;

        for (const RVertexAttribute& attr : slot.Attributes)
        {
            u32 attrSize;
            VkFormat format = DeriveVKVertexAttributeFormat(attr.Type, &attrSize);

            VkVertexInputAttributeDescription inputAttr{};
            inputAttr.binding = slotIdx;
            inputAttr.location = attr.Location;
            inputAttr.format = format;
            inputAttr.offset = stride;
            outLayout.AddAttribute(inputAttr);
            stride += attrSize;
        }

        VkVertexInputRate inputRate = slot.PollRate == RAttributePollRate::PerVertex ? VK_VERTEX_INPUT_RATE_VERTEX
                                                                                     : VK_VERTEX_INPUT_RATE_INSTANCE;

        VkVertexInputBindingDescription inputBinding{};
        inputBinding.binding = slotIdx;
        inputBinding.stride = stride;
        inputBinding.inputRate = inputRate;
        outLayout.AddBinding(inputBinding);
    }
}

VkFormat DeriveVKImageFormat(RTextureFormat format)
{
    VkFormat vkFormat = VK_FORMAT_UNDEFINED;

    for (int i = 0; i < (int)RTextureFormat::EnumCount; i++)
    {
        if (sTextureFormatMap[i].RFormat == format)
            return sTextureFormatMap[i].VKFormat;
    }

    return vkFormat;
}

RTextureFormat DeriveRTextureFormat(VkFormat vkFormat)
{
    RTextureFormat format;

    for (int i = 0; i < (int)RTextureFormat::EnumCount; i++)
    {
        if (sTextureFormatMap[i].VKFormat == vkFormat)
            return sTextureFormatMap[i].RFormat;
    }

    return format;
}

} // namespace LD
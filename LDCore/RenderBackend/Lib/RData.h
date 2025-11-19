#pragma once

#include <Ludens/RenderBackend/RBackend.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace LD {

/// @brief while RPassInfo contains transient pointer members,
///        this data representation is safe to be read at any time.
struct RPassInfoData
{
    RSampleCountBit samples = RSAMPLE_COUNT_1_BIT;
    uint32_t colorAttachmentCount = 0;
    std::vector<RPassColorAttachment> colorAttachments;
    std::vector<RPassResolveAttachment> colorResolveAttachments;
    std::optional<RPassDepthStencilAttachment> depthStencilAttachment;
    std::optional<RPassDependency> dependency;
};

} // namespace LD
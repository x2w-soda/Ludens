#pragma once

#include "Core/OS/Include/Memory.h"
#include "Core/RenderBase/Lib/RBase.h"
#include "Core/RenderBase/Include/VK/VKImage.h"

namespace LD
{

struct RDeviceVK;

struct RTextureVK : RTextureBase
{
    RTextureVK();
    RTextureVK(const RTextureVK&) = delete;
    ~RTextureVK();

    RTextureVK& operator=(const RTextureVK&) = delete;

    /// @brief create texture from existing vulkan image view, codepath for creating textures from
    ///        swap chain image views. We do not own the image, nor the image view.
    void Startup(RTexture& textureH, Ref<VKImageView> view, RDeviceVK& device);

    void Startup(RTexture& textureH, const RTextureInfo& info, RDeviceVK& device);
    void Cleanup(RTexture& textureH);

    bool UseExternalImage = false;
    VKSampler Sampler;
    VKImage Image;
    Ref<VKImageView> ImageView;
};

} // namespace LD
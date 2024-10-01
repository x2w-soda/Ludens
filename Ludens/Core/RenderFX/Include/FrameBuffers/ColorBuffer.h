#pragma once

#include "Core/RenderFX/Include/PrefabFrameBuffer.h"

namespace LD
{

class ColorPass;

struct ColorBufferInfo
{
    RDevice Device;
    ColorPass* RenderPass;
    u32 Width;
    u32 Height;
};

class ColorBuffer : public PrefabFrameBuffer
{
public:
    void Startup(const ColorBufferInfo& info);
    void Cleanup();

    RTexture GetColorAttachment() const;

private:
    RDevice mDevice;
    RTexture mColorAttachment;
};

} // namespace LD
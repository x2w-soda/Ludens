#pragma once

#include "Core/RenderFX/Include/PrefabRenderPass.h"

namespace LD
{

struct ColorPassInfo
{
    RDevice Device;
    RTextureFormat ColorFormat;
    RState InitialState = RState::Undefined;
    RState FinalState = RState::ShaderResource;
};

class ColorPass : public PrefabRenderPass
{
public:
    void Startup(const ColorPassInfo& info);
    void Cleanup();

    RTextureFormat GetColorFormat() const
    {
        return mColorFormat;
    }

private:
    RTextureFormat mColorFormat;
    RDevice mDevice;
};

} // namespace LD
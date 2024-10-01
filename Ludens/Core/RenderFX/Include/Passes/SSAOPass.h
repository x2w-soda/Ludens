#pragma once

#include "Core/RenderBase/Include/RTexture.h"
#include "Core/RenderFX/Include/PrefabRenderPass.h"

namespace LD
{

class SSAOPass : public PrefabRenderPass
{
public:
    void Startup(RDevice device);
    void Cleanup();

private:
    RDevice mDevice;
};

} // namespace LD
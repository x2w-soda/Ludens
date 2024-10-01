#pragma once

#include <Core/DSA/Include/Vector.h>
#include "Core/RenderService/Lib/RenderResources.h"

namespace LD
{

class RenderPassResources;
class GBuffer;
class SSAOBuffer;
class SSAOPass;
class ColorBuffer;
class ColorPass;

class FrameBufferResources : public RenderResources
{
public:
    void Startup(RDevice device, RenderPassResources* passRes);
    void Cleanup();

    void CreateGBuffer(GBuffer& gbuffer, int width, int height);
    void CreateSSAOBuffer(SSAOBuffer& buffer, int width, int height, SSAOPass* pass);
    void CreateColorBuffer(ColorBuffer& buffer, int width, int height, ColorPass* pass);

private:
    RenderPassResources* mPassRes;
};

} // namespace LD
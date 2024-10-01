#pragma once

#include "Core/RenderBase/Include/RTexture.h"
#include "Core/RenderService/Lib/RenderResources.h"

namespace LD
{

class TextureResources : public RenderResources
{
public:
    void Startup(RDevice device);
    void Cleanup();

    RTexture GetWhitePixel();

private:
    RTexture mWhitePixel;
};

} // namespace LD
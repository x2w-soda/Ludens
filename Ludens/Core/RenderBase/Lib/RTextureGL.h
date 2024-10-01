#pragma once

#include <glad/glad.h>
#include "Core/OS/Include/UID.h"
#include "Core/RenderBase/Include/GL/GLTexture.h"
#include "Core/RenderBase/Include/RTexture.h"
#include "Core/RenderBase/Lib/RBase.h"

namespace LD
{

struct RDeviceGL;

struct RTextureGL : RTextureBase
{
    RTextureGL();
    RTextureGL(const RTextureGL&) = delete;
    ~RTextureGL();

    RTextureGL& operator=(const RTextureGL&) = delete;

    void Startup(RTexture& textureH, const RTextureInfo& info, RDeviceGL& device);
    void Cleanup(RTexture& textureH);
    void Bind(int unit);

    GLenum Target;
    union
    {
        GLTexture2D Texture2D;
        GLTextureCube TextureCube;
    };
};

} // namespace LD
#pragma once

#include "Core/RenderBase/Lib/RBase.h"

namespace LD
{

struct RDeviceGL;

// emulate render passes on OpenGL backend
struct RPassGL : RPassBase
{
    RPassGL();
    RPassGL(const RPassGL&) = delete;
    ~RPassGL();

    RPassGL& operator=(const RPassGL&) = delete;

    void Startup(RPass& passH, const RPassInfo& info, RDeviceGL& device);
    void Cleanup(RPass& passH);
};

} // namespace LD
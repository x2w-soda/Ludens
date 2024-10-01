#include "Core/RenderBase/Lib/RPassGL.h"

namespace LD
{

RPassGL::RPassGL()
{
}

RPassGL::~RPassGL()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RPassGL::Startup(RPass& passH, const RPassInfo& info, RDeviceGL& device)
{
    RPassBase::Startup(passH, info, (RDeviceBase*)&device);

    // TODO:
}

void RPassGL::Cleanup(RPass& passH)
{
    RPassBase::Cleanup(passH);

    // TODO:
}

} // namespace LD
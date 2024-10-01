#include "Core/RenderBase/Include/RPass.h"
#include "Core/RenderBase/Lib/RBase.h"

namespace LD
{

bool RPass::HasDepthStencilAttachment()
{
    return mBase->HasDepthStencilAttachment();
}

} // namespace LD
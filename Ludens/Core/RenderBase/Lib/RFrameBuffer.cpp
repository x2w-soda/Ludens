#include "Core/RenderBase/Include/RFrameBuffer.h"
#include "Core/RenderBase/Include/RResult.h"
#include "Core/RenderBase/Lib/RBase.h"

namespace LD
{

RResult RFrameBuffer::GetColorAttachment(int idx, RTexture* colorAttachment)
{
    RResult result = mBase->GetColorAttachment(idx, colorAttachment);
    mBase->Device->Callback(result);
    return result;
}

RResult RFrameBuffer::GetDepthStencilAttachment(RTexture* depthStencilAttachment)
{
    RResult result = mBase->GetDepthStencilAttachment(depthStencilAttachment);
    mBase->Device->Callback(result);
    return result;
}

RResult RFrameBuffer::Invalidate(const RFrameBufferInfo& info)
{
    // NOTE: invalidation should not modify handle ID, since it might be cached somewhere by user code.
    UID oldID = (UID)mBase;
    RResult result = mBase->Invalidate(info);
    LD_DEBUG_ASSERT((UID)mBase == oldID);

    mBase->Device->Callback(result);
    return result;
}

} // namespace LD

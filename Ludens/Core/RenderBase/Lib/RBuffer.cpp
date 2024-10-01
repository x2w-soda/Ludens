#include "Core/RenderBase/Include/RDevice.h"
#include "Core/RenderBase/Include/RBuffer.h"
#include "Core/RenderBase/Lib/RBufferGL.h"

namespace LD
{

RResult RBuffer::SetData(u32 offset, u32 size, const void* data)
{
    return mBase->SetData(offset, size, data);
}

} // namespace LD
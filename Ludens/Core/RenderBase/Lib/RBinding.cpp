#include "Core/RenderBase/Include/RBinding.h"
#include "Core/RenderBase/Lib/RBindingGL.h"

namespace LD
{

RResult RBindingGroup::BindTexture(u32 bindingIdx, RTexture& textureH, int arrayIndex)
{
    return mBase->BindTexture(bindingIdx, textureH, arrayIndex);
}

RResult RBindingGroup::BindUniformBuffer(u32 bindingIdx, RBuffer& bufferH)
{
    return mBase->BindUniformBuffer(bindingIdx, bufferH);
}

} // namespace LD
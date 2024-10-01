#include "Core/RenderBase/Include/RResult.h"
#include "Core/Header/Include/Error.h"

namespace LD
{

const char* RResourceTypeString(RResourceType type)
{
    switch (type)
    {
    case RResourceType::Texture:
        return "RTexture";
    case RResourceType::Buffer:
        return "RBuffer";
    case RResourceType::Shader:
        return "RShader";
    case RResourceType::FrameBuffer:
        return "RFrameBuffer";
    case RResourceType::BindingGroupLayout:
        return "RBindingGroupLayout";
    case RResourceType::BindingGroup:
        return "RBindingGroup";
    case RResourceType::Pipeline:
        return "RPipeline";
    }

    LD_DEBUG_UNREACHABLE;
    return nullptr;
}

} // namespace LD
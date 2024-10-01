#include "Core/RenderBase/Include/RPipeline.h"
#include "Core/RenderBase/Lib/RBase.h"

namespace LD
{

bool RPipelineLayout::ToData(RPipelineLayoutData& data) const
{
    data.GroupLayouts.Resize(GroupLayouts.Size());
    for (size_t groupIdx = 0; groupIdx < GroupLayouts.Size(); groupIdx++)
    {
        if (!GroupLayouts[groupIdx])
            return false;

        RBindingGroupLayoutBase& srcLayout = Unwrap(GroupLayouts[groupIdx]);
        auto& dstLayout = data.GroupLayouts[groupIdx];

        dstLayout.Resize(srcLayout.Bindings.Size());
        std::copy(srcLayout.Bindings.Begin(), srcLayout.Bindings.End(), dstLayout.Begin());
    }

    return true;
}

} // namespace LD
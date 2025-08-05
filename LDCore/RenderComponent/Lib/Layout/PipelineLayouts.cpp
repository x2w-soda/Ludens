#include <Ludens/RenderComponent/Layout/PipelineLayouts.h>
#include <Ludens/RenderComponent/Layout/SetLayouts.h>
#include <array>

namespace LD {

static std::array<RSetLayoutInfo, 2> sMeshPipelineSetLayouts{
    sFrameSetLayout,
    sMaterialSetLayout,
};

RPipelineLayoutInfo sRMeshPipelineLayout{
    .setLayoutCount = sMeshPipelineSetLayouts.size(),
    .setLayouts = sMeshPipelineSetLayouts.data(),
};

} // namespace LD
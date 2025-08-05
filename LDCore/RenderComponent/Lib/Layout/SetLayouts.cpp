#include <Ludens/RenderComponent/Layout/SetLayouts.h>

namespace LD {

static RSetBindingInfo sFrameBindings[2] = {
    {0, RBINDING_TYPE_UNIFORM_BUFFER, 1},         // frame ubo
    {1, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, 1}, // environment cubemap
};

RSetLayoutInfo sFrameSetLayout = {
    .bindingCount = 2,
    .bindings = sFrameBindings,
};

static RSetBindingInfo sMaterialSetBindings[4]{
    {0, RBINDING_TYPE_UNIFORM_BUFFER, 1},
    {1, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, 1},
    {2, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, 1},
    {3, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, 1},
};

RSetLayoutInfo sMaterialSetLayout = {
    .bindingCount = 4,
    .bindings = sMaterialSetBindings,
};

static RSetBindingInfo sDoubleSampleSetBindings[2] = {
    {0, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, 1},
    {1, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, 1},
};

RSetLayoutInfo sSingleSampleSetLayout = {
    .bindingCount = 1,
    .bindings = sDoubleSampleSetBindings,
};

RSetLayoutInfo sDoubleSampleSetLayout = {
    .bindingCount = 2,
    .bindings = sDoubleSampleSetBindings,
};

} // namespace LD
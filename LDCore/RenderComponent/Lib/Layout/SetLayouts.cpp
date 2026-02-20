#include <Ludens/RenderComponent/Layout/SetLayouts.h>

namespace LD {

static_assert(sizeof(ViewProjectionData) == 208);
static_assert(alignof(ViewProjectionData) == 16);
static_assert(offsetof(ViewProjectionData, viewMat) == 0);
static_assert(offsetof(ViewProjectionData, projMat) == 64);
static_assert(offsetof(ViewProjectionData, viewProjMat) == 128);
static_assert(offsetof(ViewProjectionData, viewPos) == 192);

// hard 16KB limit for UBOs
static_assert(sizeof(FrameUBO) <= 16384);

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

void FrameUBOManager::reset(const Vec2& screenExntent, const Vec2& sceneExtent)
{
    mVPIndex = 0;
    mUBO.envPhase = 0.0f;
    mUBO.screenExtent = screenExntent;
    mUBO.sceneExtent = sceneExtent;
    mUBO.dirLight = {};
}

int FrameUBOManager::register_vp(const ViewProjectionData& vp)
{
    constexpr size_t vpCount = sizeof(mUBO.vp) / sizeof(ViewProjectionData);

    if (mVPIndex == (int)vpCount)
        return -1;

    mUBO.vp[mVPIndex] = vp;

    return mVPIndex++;
}

} // namespace LD
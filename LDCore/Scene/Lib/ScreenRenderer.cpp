#include <Ludens/Profiler/Profiler.h>

#include "ScreenRenderer.h"

namespace LD {

void ScreenRenderer::startup()
{
    mLayer = ScreenLayer::create();
}

void ScreenRenderer::cleanup()
{
    if (!mLayer)
        return;

    ScreenLayer::destroy(mLayer);
    mLayer = {};
}

void ScreenRenderer::render(DataRegistry registry)
{
    LD_PROFILE_SCOPE;

    mLayer.begin();

    for (auto ite = registry.get_components(COMPONENT_TYPE_SPRITE_2D); ite; ++ite)
    {
        Sprite2DComponent* spriteC = (Sprite2DComponent*)ite.data();

        mLayer.add_image(spriteC->transform, spriteC->local, spriteC->image, spriteC->zDepth);
    }

    mLayer.end();
}

} // namespace LD
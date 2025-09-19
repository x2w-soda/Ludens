#include <Ludens/Header/Color.h>
#include <Ludens/UI/UIAnimation.h>

#define ANIM_NONE 0
#define ANIM_SHOWING 1
#define ANIM_HIDING 2

namespace LD {

void UIOpacityAnimation::showing(float duration)
{
    mState = ANIM_SHOWING;
    mOpacity.set(duration);
}

void UIOpacityAnimation::hiding(float duration)
{
    mState = ANIM_HIDING;
    mOpacity.set(duration);
}

bool UIOpacityAnimation::is_hiding()
{
    return mState == ANIM_HIDING;
}

bool UIOpacityAnimation::update(float delta)
{
    mOpacity.update(delta);

    if (mState != ANIM_NONE && !mOpacity.is_animated())
    {
        mState = ANIM_NONE;
        return true; // animation ended
    }

    return false;
}

float UIOpacityAnimation::get_opacity()
{
    float alpha = mOpacity.get();

    switch (mState)
    {
    case ANIM_SHOWING:
        return alpha;
    case ANIM_HIDING:
        return 1.0f - alpha;
    }

    return 1.0f;
}

uint32_t UIOpacityAnimation::get_color_mask()
{
    Color mask(0xFFFFFFFF);

    if (mState != ANIM_NONE)
        mask.set_alpha(get_opacity());

    return mask;
}

} // namespace LD
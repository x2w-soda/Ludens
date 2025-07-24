#pragma once

#include <algorithm>

namespace LD {

template <typename TInterpolate>
class UIAnimation
{
public:
    /// @brief explict reset that cancels current animation
    inline void reset(float value)
    {
        mValue = value;
        mTime = -1.0f;
    }

    /// @brief begin animation
    /// @param duration animation duration in seconds
    /// @param keepValue if true, retain current animated value instead of resetting.
    inline void set(float duration)
    {
        mValue = TInterpolate::eval(0.0f);
        mTime = 0.0f;
        mDuration = duration;
    }

    /// @brief get animation value
    /// @return normalized animation value between [0.0f, 1.0f]
    inline float get() const
    {
        return mValue;
    }

    /// @brief drive the animation with delta time
    /// @param delta delta time in seconds
    inline void update(float delta)
    {
        if (mTime < 0.0f)
            return;

        mTime += delta;

        bool isComplete = false;

        if (mTime > mDuration)
        {
            mTime = mDuration;
            isComplete = true;
        }

        mValue = TInterpolate::eval(mTime / mDuration);

        if (isComplete)
            mTime = -1.0f;
    }

private:
    float mValue = 0.0f;
    float mTime = -1.0f;
    float mDuration;
};

struct LinearInterpolation
{
    static float eval(float ratio)
    {
        return std::clamp(ratio, 0.0f, 1.0f);
    }
};

struct QuadraticInterpolation
{
    static float eval(float ratio)
    {
        ratio = std::clamp(ratio, 0.0f, 1.0f);
        return ratio * ratio;
    }
};

} // namespace LD
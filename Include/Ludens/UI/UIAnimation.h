#pragma once

#include <algorithm>

namespace LD {

template <typename TInterpolate>
class UIAnimation
{
public:
    /// @brief Explict reset that cancels current animation.
    inline void reset(float value = 0.0f)
    {
        mValue = value;
        mTime = -1.0f;
        mIsAnimated = false;
    }

    /// @brief Begin animation.
    /// @param duration Animation duration in seconds.
    inline void set(float duration)
    {
        mValue = TInterpolate::eval(0.0f);
        mTime = 0.0f;
        mDuration = duration;
        mIsAnimated = true;
    }

    /// @brief Get animation value.
    inline float get() const
    {
        return mValue;
    }

    /// @brief Drive the animation with delta time.
    /// @param delta Delta time in seconds.
    inline void update(float delta)
    {
        if (!mIsAnimated)
            return;

        mTime += delta;

        if (mTime > mDuration)
        {
            mTime = mDuration;
            mIsAnimated = false;
        }

        mValue = TInterpolate::eval(mTime / mDuration);

        if (!mIsAnimated)
            mTime = -1.0f;
    }

    /// @brief Check if animation is still in progress.
    inline bool is_animated() const
    {
        return mIsAnimated;
    }

private:
    float mValue;
    float mTime;
    float mDuration;
    bool mIsAnimated;
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
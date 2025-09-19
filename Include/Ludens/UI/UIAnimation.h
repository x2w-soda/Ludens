#pragma once

#include <algorithm>
#include <cstdint>

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

/// @brief Common opacity animation tracking 'showing' and 'hiding' states.
class UIOpacityAnimation
{
public:
    /// @brief Animate opacity until one.
    void showing(float duration);

    /// @brief Animate opacity until zero.
    void hiding(float duration);

    /// @brief Check if opacity is being reduced in animation.
    bool is_hiding();

    /// @brief Update animation with delta time in seconds.
    /// @return True if animation ended in this update.
    bool update(float delta);

    /// @brief Get current opacity value.
    float get_opacity();

    /// @brief Get color mask with current opacity value.
    uint32_t get_color_mask();

private:
    UIAnimation<QuadraticInterpolation> mOpacity;
    int mState;
};

} // namespace LD
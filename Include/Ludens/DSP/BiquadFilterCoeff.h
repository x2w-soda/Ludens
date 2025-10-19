#pragma once

namespace LD {

/// @brief Biquad filter coefficients for direct form 1.
///        y[n] = b0 x[n] + b1 x[n-1] + b2 x[n-2] - a1 y[n-1] - a2 y[n-2]
struct BiquadFilterCoeff
{
    float a1;
    float a2;
    float b0;
    float b1;
    float b2;

    /// @brief Divide five other coefficients by a0.
    void normalize(float a0);

    /// @brief Prepares coefficients for direct form 1 LPF.
    void as_low_pass_filter(float Q, float cutoffFreq, float sampleFreq);
};

struct BiquadFilterHistory
{
    float xn1; // x[n-1]
    float xn2; // x[n-2]
    float yn1; // y[n-1]
    float yn2; // y[n-2]
};

/// @brief Compute direct form 1.
inline float biquad_filter_process(const BiquadFilterCoeff& coeff, BiquadFilterHistory& history, float sample)
{
    float y = coeff.b0 * sample + coeff.b1 * history.xn1 + coeff.b2 * history.xn2 - coeff.a1 * history.yn1 - coeff.a2 * history.yn2;
    history.xn2 = history.xn1;
    history.xn1 = sample;
    history.yn2 = history.yn1;
    history.yn1 = y;
    return y;
}

} // namespace LD
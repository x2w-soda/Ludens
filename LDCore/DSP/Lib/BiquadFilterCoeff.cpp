#include <Ludens/DSP/BiquadFilterCoeff.h>
#include <Ludens/Header/Math/Math.h>

namespace LD {

void BiquadFilterCoeff::normalize(float a0)
{
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;
}

void BiquadFilterCoeff::as_low_pass_filter(float Q, float cutoffFreq, float sampleFreq)
{
    float omega = M_2_PI * cutoffFreq / sampleFreq;
    float sinO = LD_SIN(omega);
    float cosO = LD_COS(omega);
    float alpha = sinO / (2.0f * Q);
    float a0 = 1.0f + alpha;

    b0 = (1.0f - cosO) / 2.0f;
    b1 = 1.0f - cosO;
    b2 = (1.0f - cosO) / 2.0f;
    a1 = -2.0f * cosO;
    a2 = 1.0 - alpha;

    normalize(a0);
}

} // namespace LD

#pragma once

namespace LD {

// clang-format off
template <typename T>
struct TQuat
{
    T x, y, z, w;
    
    TQuat() : x((T)0), y((T)0), z((T)0), w((T)1) {}
};
// clang-format on

using Quat = TQuat<float>;

} // namespace LD
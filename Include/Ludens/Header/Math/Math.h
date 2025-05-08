#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

#define LD_PI M_PI
#define LD_PI_2 M_PI_2
#define LD_PI_4 M_PI_4

#define LD_EPSILON_F32 ((float)1e-6)
#define LD_EPSILON_F64 (1e-12)

#define LD_TO_RADIANS(DEG) (DEG * (LD_PI / 180))
#define LD_TO_DEGREES(RAD) (RAD * (180 / LD_PI))

#define LD_SIN(X) std::sin(X)
#define LD_COS(X) std::cos(X)
#define LD_TAN(X) std::tan(X)

#define LD_ABS(X) std::abs(X)
#define LD_SQRT(X) std::sqrt(X)
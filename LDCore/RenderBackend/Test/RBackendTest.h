#pragma once

#define BITMAP_MSE_TOLERANCE 0.0001

namespace LD {

bool compute_bitmap_mse(const char* lhsPath, const char* rhsPath, double& outMSE);

} // namespace LD
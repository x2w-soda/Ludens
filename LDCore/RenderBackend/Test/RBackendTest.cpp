#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "RBackendTest.h"
#include <Extra/doctest/doctest.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/RenderBackend/RBackend.h>

#include <cstdio>

using namespace LD;

namespace LD {

bool compute_bitmap_mse(const char* lhsPath, const char* rhsPath, double& outMSE)
{
    Bitmap lhsBitmap = Bitmap::create_from_path(lhsPath);
    Bitmap rhsBitmap = Bitmap::create_from_path(rhsPath);
    if (!lhsBitmap || !rhsBitmap)
        return false;

    if (!Bitmap::compute_mse(lhsBitmap.view(), rhsBitmap.view(), outMSE))
        return false;

    printf("compute_bitmap_mse: %f\n- %s\n- %s\n", outMSE, lhsPath, rhsPath);

    return true;
}

} // namespace LD

TEST_CASE("hash_pipeline_rasterization_state")
{
    RPipelineRasterizationInfo r1 = {
        .polygonMode = RPOLYGON_MODE_FILL,
        .cullMode = RCULL_MODE_BACK,
        .lineWidth = 1,
    };
    RPipelineRasterizationInfo r2(r1);

    Hash64 h1 = hash64_pipeline_rasterization_state(r1);
    Hash64 h2 = hash64_pipeline_rasterization_state(r2);
    CHECK(h1 == h2);

    r2.cullMode = RCULL_MODE_NONE;
    h2 = hash64_pipeline_rasterization_state(r2);
    CHECK(h1 != h2); // respect cull mode difference

    r1 = {
        .polygonMode = RPOLYGON_MODE_FILL,
        .cullMode = RCULL_MODE_NONE,
        .lineWidth = 1,
    };
    r2 = {
        .polygonMode = RPOLYGON_MODE_FILL,
        .cullMode = RCULL_MODE_NONE,
        .lineWidth = 3,
    };
    h1 = hash64_pipeline_rasterization_state(r1);
    h2 = hash64_pipeline_rasterization_state(r2);
    CHECK(h1 == h2); // ignore line width difference

    r1.polygonMode = RPOLYGON_MODE_LINE;
    r2.polygonMode = RPOLYGON_MODE_LINE;
    h1 = hash64_pipeline_rasterization_state(r1);
    h2 = hash64_pipeline_rasterization_state(r2);
    CHECK(h1 != h2); // respect line width difference
}

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "Extra/doctest/doctest.h"
#include <Ludens/RenderBackend/RBackend.h>

using namespace LD;

TEST_CASE("hash_pipeline_rasterization_state")
{
    RPipelineRasterizationInfo r1 = {
        .polygonMode = RPOLYGON_MODE_FILL,
        .cullMode = RCULL_MODE_BACK,
        .lineWidth = 1,
    };
    RPipelineRasterizationInfo r2(r1);

    uint32_t h1 = hash32_pipeline_rasterization_state(r1);
    uint32_t h2 = hash32_pipeline_rasterization_state(r2);
    CHECK(h1 == h2);

    r2.cullMode = RCULL_MODE_NONE;
    h2 = hash32_pipeline_rasterization_state(r2);
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
    h1 = hash32_pipeline_rasterization_state(r1);
    h2 = hash32_pipeline_rasterization_state(r2);
    CHECK(h1 == h2); // ignore line width difference

    r1.polygonMode = RPOLYGON_MODE_LINE;
    r2.polygonMode = RPOLYGON_MODE_LINE;
    h1 = hash32_pipeline_rasterization_state(r1);
    h2 = hash32_pipeline_rasterization_state(r2);
    CHECK(h1 != h2); // respect line width difference
}

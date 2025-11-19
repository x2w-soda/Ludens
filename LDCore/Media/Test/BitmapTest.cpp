#include <Extra/doctest/doctest.h>
#include <Ludens/Header/Math/Math.h>
#include <Ludens/Media/Bitmap.h>

using namespace LD;

TEST_CASE("Bitmap MSE")
{
    uint32_t pixel1 = 0xFFFFFFFF;
    uint32_t pixel2 = 0;
    BitmapView view1{1, 1, BITMAP_CHANNEL_RGBA, (const char*)&pixel1};
    BitmapView view2{1, 1, BITMAP_CHANNEL_RGBA, (const char*)&pixel2};

    double mse;
    CHECK(Bitmap::compute_mse(view1, view2, mse));
    CHECK(is_equal_epsilon(mse, 1.0));

    pixel2 = 0x00FF00FF;
    CHECK(Bitmap::compute_mse(view1, view2, mse));
    CHECK(is_equal_epsilon(mse, 0.5));

    pixel2 = 0xFF00FFFF;
    CHECK(Bitmap::compute_mse(view1, view2, mse));
    CHECK(is_equal_epsilon(mse, 0.25));

    pixel2 = pixel1;
    CHECK(Bitmap::compute_mse(view1, view2, mse));
    CHECK(is_equal_epsilon(mse, 0.0));

    CHECK(Bitmap::compute_mse(view1, view1, mse));
    CHECK(is_equal_epsilon(mse, 0.0));
}
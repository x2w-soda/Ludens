#include <doctest.h>
#include "Core/Math/Include/Mat3.h"

using namespace LD;

static const Mat3 sMat3Pattern{ { 0.0f, 1.0f, 2.0f }, { 3.0f, 4.0f, 5.0f }, { 6.0f, 7.0f, 8.0f } };

TEST_CASE("Mat3 Transpose")
{
    Mat3 t = Mat3::Transpose(sMat3Pattern);

    CHECK(t[0][0] == 0.0f);
    CHECK(t[0][1] == 3.0f);
    CHECK(t[0][2] == 6.0f);
    CHECK(t[1][0] == 1.0f);
    CHECK(t[1][1] == 4.0f);
    CHECK(t[1][2] == 7.0f);
    CHECK(t[2][0] == 2.0f);
    CHECK(t[2][1] == 5.0f);
    CHECK(t[2][2] == 8.0f);
}

TEST_CASE("Mat3 Inverse")
{
    Mat3 upper({ 1.0f, 0.0f, 0.0f }, { 0.0f, -5.0f, 0.0f }, { 5.0f, -7.0f, 9.0f });
    CHECK(upper.IsInvertible());

    Mat3 inv = Mat3::Inverse(upper);
    Mat3 I = inv * upper;
    CHECK(Mat3::Equal(I, Mat3::Identity));

    I = upper * inv;
    CHECK(Mat3::Equal(I, Mat3::Identity));

    Mat3 lower({ 1.32f, 22.0f, -33.0f }, { 0.0f, -3.14f, 20.0f }, { 0.0f, 0.0f, -44.0f });
    CHECK(lower.IsInvertible());

    inv = Mat3::Inverse(lower);
    I = lower * inv;
    bool eq = Mat3::Equal(I, Mat3::Identity);
    CHECK(eq);

    I = inv * lower;
    CHECK(Mat3::Equal(I, Mat3::Identity));
}
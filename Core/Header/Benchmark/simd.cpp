#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/Header/Math/Vec4.h>
#include <Ludens/System/Timer.h>
#include <cstdio>
#include <vector>

using namespace LD;

constexpr size_t N = 10'000'000;

int main(int argc, char** argv)
{
    size_t dur;
    {
        std::vector<Vec4> a(N, Vec4(1, 2, 3, 4));
        std::vector<Vec4> b(N, Vec4(5, 6, 7, 8));
        std::vector<Vec4> c(N);

        ScopeTimer timer(&dur);

        for (size_t i = 0; i < N; ++i)
        {
            c[i] = a[i] * b[i];
        }
    }
    printf("Vec4  mul %.3f ms\n", dur / 1000.0f);

    {
        std::vector<DVec4> a(N, DVec4(1, 2, 3, 4));
        std::vector<DVec4> b(N, DVec4(5, 6, 7, 8));
        std::vector<DVec4> c(N);

        ScopeTimer timer(&dur);

        for (size_t i = 0; i < N; ++i)
        {
            c[i] = a[i] * b[i];
        }
    }
    printf("DVec4 mul %.3f ms\n", dur / 1000.0f);

    {
        std::vector<Mat4> a(N, Mat4(2));
        std::vector<Mat4> b(N, Mat4(3));
        std::vector<Mat4> c(N);

        ScopeTimer timer(&dur);

        for (size_t i = 0; i < N; ++i)
        {
            c[i] = a[i] * b[i];
        }
    }
    printf("Mat4  mul %.3f ms\n", dur / 1000.0f);

    {
        std::vector<DMat4> a(N, DMat4(2));
        std::vector<DMat4> b(N, DMat4(3));
        std::vector<DMat4> c(N);

        ScopeTimer timer(&dur);

        for (size_t i = 0; i < N; ++i)
        {
            c[i] = a[i] * b[i];
        }
    }
    printf("DMat4 mul %.3f ms\n", dur / 1000.0f);
}
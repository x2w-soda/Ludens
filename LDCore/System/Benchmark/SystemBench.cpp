#include <Ludens/System/Allocator.h>
#include <Ludens/System/Timer.h>
#include <cstdio>
#include <random>
#include <vector>
#include <Ludens/DataRegistry/DataComponent.h>

using namespace LD;
using Scalar = uint32_t;

constexpr size_t N = 1'000'000;
static Scalar sA[N];
static Scalar* sB[N];
static TransformComponent tA[N];
static TransformComponent* tB[N];

int main(int argc, char** argv)
{
    PoolAllocatorInfo paI{};
    paI.blockSize = sizeof(Scalar);
    paI.pageSize = 8192;
    paI.isMultiPage = true;
    PoolAllocator sC = PoolAllocator::create(paI);
    paI.blockSize = sizeof(TransformComponent);
    paI.pageSize = 8192;
    paI.isMultiPage = true;
    PoolAllocator tC = PoolAllocator::create(paI);
    std::vector<size_t> randI(N);

    for (size_t i = 0; i < N; i++)
    {
        sA[i] = i;
        sB[i] = new Scalar(i);
        tB[i] = new TransformComponent();
        *(Scalar*)sC.allocate() = i;
        tC.allocate();
        randI[i] = i;
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(randI.begin(), randI.end(), g);

    // NOTE: Scalar benchmarks are likely vectorized,
    //       check the disassembly for SSE instructions
    //       if some cases seem absurdly fast.

    size_t dur;
    {
        ScopeTimer timer(&dur);

        for (size_t i = 0; i < N; ++i)
        {
            sA[i]++;
        }
    }
    printf("Scalar Array %.3f ms\n", dur / 1000.0f);

    {
        ScopeTimer timer(&dur);

        for (size_t i = 0; i < N; ++i)
        {
            (*sB[i])++;
        }
    }
    printf("Scalar Array of ptr %.3f ms\n", dur / 1000.0f);

    {
        ScopeTimer timer(&dur);

        for (auto ite = sC.begin(); ite; ++ite)
        {
            (*(Scalar*)ite.data())++;
        }
    }
    printf("Scalar PoolAllocator %.3f ms\n", dur / 1000.0f);

    //
    // iterate TransformComponents
    //

    {
        ScopeTimer timer(&dur);

        for (size_t i = 0; i < N; ++i)
        {
            tA[i].transform.position = Vec3(1);
            tA[i].transform.rotation = Vec3(2);
            tA[i].transform.scale = Vec3(3);
        }
    }
    printf("TransformComponent Array %.3f ms\n", dur / 1000.0f);

    {
        ScopeTimer timer(&dur);

        for (size_t i = 0; i < N; ++i)
        {
            size_t idx = randI[i];
            tA[idx].transform.position = Vec3(1);
            tA[idx].transform.rotation = Vec3(2);
            tA[idx].transform.scale = Vec3(3);
        }
    }
    printf("TransformComponent Array Random Access %.3f ms\n", dur / 1000.0f);

    {
        ScopeTimer timer(&dur);

        for (size_t i = 0; i < N; ++i)
        {
            tB[i]->transform.position = Vec3(1);
            tB[i]->transform.rotation = Vec3(2);
            tB[i]->transform.scale = Vec3(3);
        }
    }
    printf("TransformComponent Array of Ptr %.3f ms\n", dur / 1000.0f);

    {
        ScopeTimer timer(&dur);

        for (size_t i = 0; i < N; ++i)
        {
            size_t idx = randI[i];
            tB[idx]->transform.position = Vec3(1);
            tB[idx]->transform.rotation = Vec3(2);
            tB[idx]->transform.scale = Vec3(3);
        }
    }
    printf("TransformComponent Array of Ptr Random Access %.3f ms\n", dur / 1000.0f);

    {
        ScopeTimer timer(&dur);
        for (auto ite = tC.begin(); ite; ++ite)
        {
            TransformComponent* t = (TransformComponent*)ite.data();
            t->transform.position = Vec3(1);
            t->transform.rotation = Vec3(2);
            t->transform.scale = Vec3(3);
        }
    }
    printf("TransformComponent PoolAllocator %.3f ms\n", dur / 1000.0f);
}
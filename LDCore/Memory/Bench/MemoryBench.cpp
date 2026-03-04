#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Memory/Allocator.h>
#include <Ludens/System/Timer.h>

#include <cstdio>
#include <random>
#include <vector>

using namespace LD;
using Scalar = uint32_t;

constexpr size_t N = 1'000'000;
static Scalar scalarArray[N];
static Scalar* scalarPtrArray[N];
static Transform2DComponent transformArray[N];
static Transform2DComponent* transformPtrArray[N];
static Sprite2DComponent sprite2DArray[N];

int main(int argc, char** argv)
{
    PoolAllocatorInfo paI{};
    paI.blockSize = sizeof(Scalar);
    paI.pageSize = 8192;
    paI.isMultiPage = true;
    PoolAllocator sC = PoolAllocator::create(paI);

    paI.blockSize = sizeof(Transform2DComponent);
    PoolAllocator transform2DCPA = PoolAllocator::create(paI);

    paI.blockSize = sizeof(Sprite2DComponent);
    PoolAllocator sprite2DCPA = PoolAllocator::create(paI);

    paI.blockSize = sizeof(Transform2D);
    PoolAllocator transform2DPA = PoolAllocator::create(paI);

    std::vector<size_t> randI(N);

    for (size_t i = 0; i < N; i++)
    {
        scalarArray[i] = i;
        scalarPtrArray[i] = new Scalar(i);
        transformArray[i].transform = (Transform2D*)transform2DPA.allocate();
        transformPtrArray[i] = new Transform2DComponent();
        transformPtrArray[i]->transform = (Transform2D*)transform2DPA.allocate();
        sprite2DArray[i].transform = (Transform2D*)transform2DPA.allocate();
        *(Scalar*)sC.allocate() = i;
        randI[i] = i;
        Transform2DComponent* transform2DC = (Transform2DComponent*)transform2DCPA.allocate();
        transform2DC->transform = (Transform2D*)transform2DPA.allocate();
        Sprite2DComponent* sprite2DC = (Sprite2DComponent*)sprite2DCPA.allocate();
        sprite2DC->transform = (Transform2D*)transform2DPA.allocate();
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
            scalarArray[i]++;
        }
    }
    printf("Scalar Array %.3f ms\n", dur / 1000.0f);

    {
        ScopeTimer timer(&dur);

        for (size_t i = 0; i < N; ++i)
        {
            (*scalarPtrArray[i])++;
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
            transformArray[i].transform->position = Vec2(1.0f);
            transformArray[i].transform->rotation = 2.0f;
            transformArray[i].transform->scale = Vec2(3.0f);
        }
    }
    printf("TransformComponent Array %.3f ms\n", dur / 1000.0f);

    {
        ScopeTimer timer(&dur);

        for (size_t i = 0; i < N; ++i)
        {
            size_t idx = randI[i];
            transformArray[idx].transform->position = Vec2(1.0f);
            transformArray[idx].transform->rotation = 2.0f;
            transformArray[idx].transform->scale = Vec2(3.0f);
        }
    }
    printf("TransformComponent Array Random Access %.3f ms\n", dur / 1000.0f);

    {
        ScopeTimer timer(&dur);

        for (size_t i = 0; i < N; ++i)
        {
            transformPtrArray[i]->transform->position = Vec2(1.0f);
            transformPtrArray[i]->transform->rotation = 2.0f;
            transformPtrArray[i]->transform->scale = Vec2(3.0f);
        }
    }
    printf("TransformComponent Array of Ptr %.3f ms\n", dur / 1000.0f);

    {
        ScopeTimer timer(&dur);

        for (size_t i = 0; i < N; ++i)
        {
            size_t idx = randI[i];
            transformPtrArray[idx]->transform->position = Vec2(1.0f);
            transformPtrArray[idx]->transform->rotation = 2.0f;
            transformPtrArray[idx]->transform->scale = Vec2(3.0f);
        }
    }
    printf("TransformComponent Array of Ptr Random Access %.3f ms\n", dur / 1000.0f);

    {
        ScopeTimer timer(&dur);
        for (auto ite = transform2DCPA.begin(); ite; ++ite)
        {
            Transform2DComponent* t = (Transform2DComponent*)ite.data();
            t->transform->position = Vec2(1);
            t->transform->rotation = 2.0f;
            t->transform->scale = Vec2(3);
        }
    }
    printf("TransformComponent PoolAllocator %.3f ms\n", dur / 1000.0f);

    {
        ScopeTimer timer(&dur);

        for (size_t i = 0; i < N; ++i)
        {
            sprite2DArray[i].transform->position = Vec2(1.0f);
            sprite2DArray[i].transform->rotation = 2.0f;
            sprite2DArray[i].transform->scale = Vec2(3.0f);
        }
    }
    printf("Sprite2DComponent Array %.3f ms\n", dur / 1000.0f);

    {
        ScopeTimer timer(&dur);
        for (auto ite = sprite2DCPA.begin(); ite; ++ite)
        {
            Sprite2DComponent* t = (Sprite2DComponent*)ite.data();
            t->transform->position = Vec2(1.0f);
            t->transform->rotation = 2.0f;
            t->transform->scale = Vec2(3.0f);
        }
    }
    printf("Sprite2DComponent PoolAllocator %.3f ms\n", dur / 1000.0f);
}
#include <Extra/doctest/doctest.h>
#include <Ludens/DataRegistry/TransformRegistry.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/System/Timer.h>

using namespace LD;

const uint64_t N = 1000;
static_assert(N % 2 == 0);

Transform2D translate(const Vec2& offset)
{
    Transform2D tr{};
    tr.position = offset;
    tr.rotation = 0.0f;
    tr.scale = Vec2(1.0f);
    return tr;
}

static void chain_hierarchy(ID id, Vector<ID>& children, void*)
{
    if ((uint64_t)id < N)
        children.push_back((uint64_t)id + 1);
};

TEST_CASE("Transform2DRegistry root")
{
    Transform2DRegistry* reg = new Transform2DRegistry;
    CHECK_FALSE(reg->has_transform(1));
    reg->create(1, 0);
    CHECK(reg->has_transform(1));
    reg->set_transform(1, translate(Vec2(3.0f, 4.0f)));
    reg->invalidate_transforms();

    Mat4 mat4 = reg->get_world_mat4(1);
    Vec4 v(0.0f, 0.0f, 0.0f, 1.0f);
    Vec4 v2 = mat4 * v;
    CHECK(v2 == Vec4(3.0, 4.0f, 0.0f, 1.0f));

    reg->set_transform(1, translate(Vec2(2.0, 3.0f)));
    reg->invalidate_transforms();
    mat4 = reg->get_world_mat4(1);
    v2 = mat4 * v;
    CHECK(v2 == Vec4(2.0, 3.0f, 0.0f, 1.0f));

    delete reg;
    CHECK(get_memory_leaks(nullptr) == 0);
}

TEST_CASE("Transform2DRegistry wide depth 0")
{
    Transform2DRegistry* reg = new Transform2DRegistry;

    for (uint64_t i = 1; i <= N; i++)
    {
        reg->create(i, 0);
        reg->set_transform(i, translate(Vec2(i * 1.0f, i * 2.0f)));
    }

    size_t us;
    {
        ScopeTimer scope(&us);
        reg->invalidate_transforms();
    }
    printf("invalidate_transforms %f\n", us / 1000.0f);

    for (uint64_t i = 1; i <= N; i++)
    {
        Vec4 v = reg->get_world_mat4(i) * Vec4(0.0f, 0.0f, 0.0f, 1.0f);
        CHECK(v == Vec4(i, i * 2.0f, 0.0f, 1.0f));
    }

    delete reg;
    CHECK(get_memory_leaks(nullptr) == 0);
}

TEST_CASE("Transform2DRegistry wide depth 1")
{
    Transform2DRegistry* reg = new Transform2DRegistry;

    reg->create(1, 0);
    reg->set_transform(1, translate(Vec2(1.0f, 1.0f)));

    for (uint64_t i = 2; i <= N; i++)
    {
        reg->create(i, 1);
        reg->set_transform(i, translate(Vec2(2.0f, 2.0f)));
    }

    size_t us;
    {
        ScopeTimer scope(&us);
        reg->invalidate_transforms();
    }
    printf("invalidate_transforms depth 1 %f\n", us / 1000.0f);

    for (uint64_t i = 2; i <= N; i++)
    {
        Vec4 v = reg->get_world_mat4(i) * Vec4(0.0f, 0.0f, 0.0f, 1.0f);
        CHECK(v == Vec4(3.0f, 3.0f, 0.0f, 1.0f));
    }

    reg->set_transform(1, translate(Vec2(3.0f, 3.0f)));

    {
        ScopeTimer scope(&us);
        reg->invalidate_transforms();
    }
    printf("invalidate_transforms depth 1 %f\n", us / 1000.0f);

    for (uint64_t i = 2; i <= N; i++)
    {
        Vec4 v = reg->get_world_mat4(i) * Vec4(0.0f, 0.0f, 0.0f, 1.0f);
        CHECK(v == Vec4(5.0f, 5.0f, 0.0f, 1.0f));
    }

    delete reg;
    CHECK(get_memory_leaks(nullptr) == 0);
}

TEST_CASE("Transform2DRegistry skewed")
{
    Transform2DRegistry* reg = new Transform2DRegistry;
    reg->create(1, 0);
    reg->set_transform(1, translate(Vec2(1.0f, 2.0f)));

    for (uint64_t i = 2; i <= N; i++)
    {
        reg->create(i, i - 1);
        reg->set_transform(i, translate(Vec2(1.0f, 2.0f)));
    }

    size_t us;
    {
        ScopeTimer scope(&us);
        reg->invalidate_transforms();
    }
    printf("invalidate_transforms %f\n", us / 1000.0f);

    Mat4 mat4 = reg->get_world_mat4(N);
    Vec4 v = mat4 * Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    CHECK(v == Vec4(N, 2.0f * N, 0.0f, 1.0f));

    reg->set_transform(N / 2, translate(Vec2(2.0f, 3.0f)));
    reg->invalidate_transforms();
    mat4 = reg->get_world_mat4(N);
    v = mat4 * Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    CHECK(v == Vec4(N + 1.0f, 2 * N + 1.0f, 0.0f, 1.0f));

    // destruction

    reg->destroy(1, &chain_hierarchy, nullptr);
    for (uint64_t i = 1; i <= N; i++)
        CHECK_FALSE(reg->has_transform(i));

    reg->destroy(1, &chain_hierarchy, nullptr);

    delete reg;
    CHECK(get_memory_leaks(nullptr) == 0);
}

TEST_CASE("Transform2DRegistry reparent")
{
    Transform2DRegistry* reg = new Transform2DRegistry;
    reg->create(1, 0);
    reg->set_transform(1, translate(Vec2(1.0f, 2.0f)));

    // start with single chain
    for (uint64_t i = 2; i <= N; i++)
    {
        reg->create(i, i - 1);
        reg->set_transform(i, translate(Vec2(1.0f, 2.0f)));
    }

    reg->reparent(N / 2 + 1, 0, &chain_hierarchy, nullptr);

    // two chains with length N/2 starting at ID 1 and N/2 + 1
    size_t us;
    {
        ScopeTimer scope(&us);
        reg->invalidate_transforms();
    }
    printf("invalidate_transforms %f\n", us / 1000.0f);

    Mat4 mat4 = reg->get_world_mat4(N / 2);
    Vec4 v = mat4 * Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    CHECK(v == Vec4(N / 2, N, 0.0f, 1.0f));

    mat4 = reg->get_world_mat4(N);
    v = mat4 * Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    CHECK(v == Vec4(N / 2, N, 0.0f, 1.0f));

    delete reg;
    CHECK(get_memory_leaks(nullptr) == 0);
}
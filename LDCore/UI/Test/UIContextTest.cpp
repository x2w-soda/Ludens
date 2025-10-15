#include "UITest.h"
#include <Extra/doctest/doctest.h>

TEST_CASE("UIContext layers sanity check")
{
    UIContext ctx = UITest::create_test_context();

    const Hash32 testLayerHash((uint32_t)0);
    CHECK(ctx.has_layer(testLayerHash)); // added during test context creation

    constexpr Hash32 layer1Hash("SomeLayer1");
    CHECK(!ctx.has_layer(layer1Hash));

    ctx.add_layer(layer1Hash);
    CHECK(ctx.has_layer(layer1Hash));

    std::vector<Hash32> layers;
    ctx.get_layers(layers);
    CHECK(layers.size() == 2);
    CHECK(layers[0] == testLayerHash);
    CHECK(layers[1] == layer1Hash);

    ctx.raise_layer(testLayerHash);
    ctx.get_layers(layers);
    CHECK(layers.size() == 2);
    CHECK(layers[0] == layer1Hash);
    CHECK(layers[1] == testLayerHash);

    constexpr Hash32 layer2Hash("SomeLayer2");
    ctx.add_layer(layer2Hash);
    ctx.get_layers(layers);
    CHECK(layers.size() == 3);
    CHECK(layers[0] == layer1Hash);
    CHECK(layers[1] == testLayerHash);
    CHECK(layers[2] == layer2Hash);

    ctx.remove_layer(testLayerHash);
    CHECK(!ctx.has_layer(testLayerHash));
    ctx.get_layers(layers);
    CHECK(layers.size() == 2);
    CHECK(layers[0] == layer1Hash);
    CHECK(layers[1] == layer2Hash);

    ctx.remove_layer(layer1Hash);
    CHECK(!ctx.has_layer(layer1Hash));
    ctx.get_layers(layers);
    CHECK(layers.size() == 1);
    CHECK(layers[0] == layer2Hash);

    UIContext::destroy(ctx);
    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_UI);
    CHECK(profile.current == 0);
}
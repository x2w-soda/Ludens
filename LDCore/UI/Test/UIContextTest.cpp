#include "UITest.h"
#include <Extra/doctest/doctest.h>

TEST_CASE("UIContext layers sanity check")
{
    UIWorkspace space;
    UIContext ctx = UITest::create_test_context(Vec2(100.0f, 100.0f), space);
    ctx.create_layer("layer1");

    Vector<UILayer> layers;
    ctx.get_layers(layers);
    CHECK(layers.size() == 2);

    UILayer l0 = layers[0];
    UILayer l1 = layers[1];
    l0.raise();

    ctx.get_layers(layers);
    CHECK(layers.size() == 2);
    CHECK(layers[0].unwrap() == l1.unwrap());
    CHECK(layers[1].unwrap() == l0.unwrap());

    UILayer l2 = ctx.create_layer("layer2");
    ctx.get_layers(layers);
    CHECK(layers.size() == 3);
    CHECK(layers[0].unwrap() == l1.unwrap());
    CHECK(layers[1].unwrap() == l0.unwrap());
    CHECK(layers[2].unwrap() == l2.unwrap());

    ctx.destroy_layer(l0);
    ctx.get_layers(layers);
    CHECK(layers.size() == 2);
    CHECK(layers[0].unwrap() == l1.unwrap());
    CHECK(layers[1].unwrap() == l2.unwrap());

    ctx.destroy_layer(l1);
    ctx.get_layers(layers);
    CHECK(layers.size() == 1);
    CHECK(layers[0].unwrap() == l2.unwrap());

    UIContext::destroy(ctx);
    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_UI);
    CHECK(profile.current == 0);
}
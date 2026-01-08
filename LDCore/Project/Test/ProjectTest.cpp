#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>

#include <Ludens/Project/ProjectSettings.h>
using namespace LD;

TEST_CASE("ProjectScreenLayerSettings")
{
    ProjectSettings projectS = ProjectSettings::create();
    ProjectScreenLayerSettings layerS = projectS.get_screen_layer_settings();
    Vector<ProjectScreenLayer> order = layerS.get_layers();
    CHECK(order.empty());

    ProjectScreenLayerID layer1 = layerS.create_layer("default");
    order = layerS.get_layers();
    CHECK(order.size() == 1);
    CHECK(order[0].id == layer1);
    CHECK(order[0].name == "default");
    
    ProjectScreenLayerID layer2 = layerS.create_layer("foreground");
    order = layerS.get_layers();
    CHECK(order.size() == 2);
    CHECK(order[0].id == layer1);
    CHECK(order[0].name == "default");
    CHECK(order[1].id == layer2);
    CHECK(order[1].name == "foreground");

    layerS.rename_layer(layer2, "fg");
    layerS.rotate_layer(layer2, 0);
    order = layerS.get_layers();
    CHECK(order.size() == 2);
    CHECK(order[0].id == layer2);
    CHECK(order[0].name == "fg");
    CHECK(order[1].id == layer1);
    CHECK(order[1].name == "default");

    ProjectSettings::destroy(projectS);
}
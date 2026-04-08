#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>

#include <Ludens/Project/ProjectSettings.h>
using namespace LD;

TEST_CASE("ProjectScreenLayerSettings")
{
    SUIDRegistry idReg = SUIDRegistry::create();
    ProjectSettings projectS = ProjectSettings::create();
    ProjectScreenLayerSettings layerS = projectS.screen_layer_settings();
    Vector<ProjectScreenLayer> order = layerS.get_layers();
    CHECK(order.empty());

    SUID layer1 = layerS.create_layer(idReg , "default");
    order = layerS.get_layers();
    CHECK(order.size() == 1);
    CHECK(order[0].id == layer1);
    CHECK(order[0].name == "default");
    
    SUID layer2 = layerS.create_layer(idReg, "foreground");
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
    SUIDRegistry::destroy(idReg);
}
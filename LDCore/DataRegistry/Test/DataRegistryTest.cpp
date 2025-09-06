#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>

#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/DataRegistry/DataRegistry.h>

using namespace LD;

TEST_CASE("DataRegistry")
{
    DataRegistry reg = DataRegistry::create();

    CUID t1 = reg.create_component(COMPONENT_TYPE_TRANSFORM, "t1", (CUID)0, (CUID)0);

    ComponentType type;
    auto* comp = (TransformComponent*)reg.get_component(t1, type);
    CHECK(type == COMPONENT_TYPE_TRANSFORM);
    comp->transform.position = Vec3(0.0f);
    comp->transform.rotation = Vec3(1.0f, 2.0f, 3.0f);
    comp->transform.scale = Vec3(1.0f);

    DataRegistry::destroy(reg);
}

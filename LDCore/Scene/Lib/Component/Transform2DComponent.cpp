#include <Ludens/Scene/Component/Transform2DView.h>
#include <Ludens/Serial/Property.h>

#include "Transform2DComponent.h"

namespace LD {

// clang-format off
static PropertyMeta sTransform2DPropMeta[] = {
    {"transform", VALUE_TYPE_TRANSFORM_2D},
};
// clang-format on

void transform_2d_prop_getter(void* data, uint32_t index, Value64& val)
{
    Transform2DView view((ComponentBase**)data);
    Transform2D transform2D;

    switch (index)
    {
    case TRANSFORM_2D_PROP_TRANSFORM:
        (void)view.get_transform_2d(transform2D);
        val.set_transform_2d(transform2D);
        break;
    default:
        break;
    }
}

void transform_2d_prop_setter(void* data, uint32_t index, const Value64& val)
{
    Transform2DView view((ComponentBase**)data);

    switch (index)
    {
    case TRANSFORM_2D_PROP_TRANSFORM:
        view.set_transform_2d(val.get_transform_2d());
        break;
    default:
        break;
    }
}

PropertyMetaTable gTransform2DPropMetaTable{
    .entries = sTransform2DPropMeta,
    .entryCount = sizeof(sTransform2DPropMeta) / sizeof(*sTransform2DPropMeta),
    .getter = &transform_2d_prop_getter,
    .setter = &transform_2d_prop_setter,
};

void init_transform_2d_component(ComponentBase** dstData)
{
    ComponentBase* base = *dstData;
    Transform2DComponent* dstTransform2D = (Transform2DComponent*)dstData;
    dstTransform2D->transform = base->transform2D;
}

bool clone_transform_2d_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err)
{
    Transform2DComponent* dstTransform2D = (Transform2DComponent*)dstData;
    Transform2DComponent* srcTransform2D = (Transform2DComponent*)srcData;
    dstTransform2D->transform = srcTransform2D->transform;

    return true;
}

} // namespace LD
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

void Transform2DMeta::init(ComponentBase** dstData)
{
    ComponentBase* base = *dstData;
    Transform2DComponent* dstTransform2D = (Transform2DComponent*)dstData;
    dstTransform2D->transform = base->transform2D;
}

bool Transform2DMeta::clone(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err)
{
    Transform2DComponent* dstTransform2D = (Transform2DComponent*)dstData;
    Transform2DComponent* srcTransform2D = (Transform2DComponent*)srcData;
    dstTransform2D->transform = srcTransform2D->transform;

    return true;
}

bool Transform2DMeta::load_from_props(SceneObj* scene, ComponentBase** data, const Vector<PropertyValue>& props, std::string& err)
{
    Transform2D transform2D = Transform2D::identity();

    for (const PropertyValue& prop : props)
    {
        switch (prop.index)
        {
        case TRANSFORM_2D_PROP_TRANSFORM:
            transform2D = prop.value.get_transform_2d();
            break;
        }
    }

    auto* dstTransform2D = (Transform2DComponent*)data;
    *dstTransform2D->transform = transform2D;

    return true;
}

} // namespace LD
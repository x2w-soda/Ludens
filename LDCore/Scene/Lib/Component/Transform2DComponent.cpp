#include <Ludens/Scene/Component/Transform2DView.h>
#include <Ludens/Serial/Property.h>

#include "Transform2DComponent.h"

namespace LD {

// clang-format off
static PropertyMeta sTransform2DPropMeta[] = {
    {"transform", nullptr, VALUE_TYPE_TRANSFORM_2D},
};
// clang-format on

bool transform_2d_prop_getter(void* data, uint32_t propIndex, uint32_t arrayIndex, Value64& val)
{
    Transform2DView view((ComponentBase**)data);
    Transform2D transform2D;

    switch (propIndex)
    {
    case TRANSFORM_2D_PROP_TRANSFORM:
        (void)view.get_transform_2d(transform2D);
        val.set_transform_2d(transform2D);
        break;
    default:
        return false;
    }

    return true;
}

bool transform_2d_prop_setter(void* data, uint32_t propIndex, uint32_t arrayIndex, const Value64& val)
{
    Transform2DView view((ComponentBase**)data);

    switch (propIndex)
    {
    case TRANSFORM_2D_PROP_TRANSFORM:
        view.set_transform_2d(val.get_transform_2d());
        break;
    default:
        return false;
    }

    return true;
}

TypeMeta Transform2DMeta::sTypeMeta = {
    .name = "Transform2DComponent",
    .props = sTransform2DPropMeta,
    .propCount = sizeof(sTransform2DPropMeta) / sizeof(*sTransform2DPropMeta),
    .getLocal = &transform_2d_prop_getter,
    .setLocal = &transform_2d_prop_setter,
};

void Transform2DMeta::init(ComponentBase** dstData)
{
    ComponentBase* base = *dstData;
    Transform2DComponent* dstTransform2D = (Transform2DComponent*)dstData;
    dstTransform2D->transform = base->transform2D;
}

bool Transform2DMeta::clone(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, String& err)
{
    Transform2DComponent* dstTransform2D = (Transform2DComponent*)dstData;
    Transform2DComponent* srcTransform2D = (Transform2DComponent*)srcData;
    dstTransform2D->transform = srcTransform2D->transform;

    return true;
}

bool Transform2DMeta::load_from_props(SceneObj* scene, ComponentBase** data, const Vector<PropertyValue>& props, String& err)
{
    Transform2D transform2D = Transform2D::identity();

    for (const PropertyValue& prop : props)
    {
        switch (prop.propIndex)
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
#include <Ludens/Scene/Component/Transform2DView.h>

#include "Transform2DComponent.h"

namespace LD {

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
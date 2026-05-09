#pragma once

#include "../SceneObj.h"

namespace LD {

struct Transform2DMeta
{
    static void init(ComponentBase** dstData);
    static bool clone(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, String& err);
    static bool load_from_props(SceneObj* scene, ComponentBase** data, const Vector<PropertyValue>& props, String& err);

    static TypeMeta sTypeMeta;
};

} // namespace LD
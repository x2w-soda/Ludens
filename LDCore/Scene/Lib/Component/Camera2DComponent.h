#pragma once

#include "../SceneObj.h"

namespace LD {

struct Camera2DMeta
{
    static TypeMeta sTypeMeta;

    static void init(ComponentBase** dstData);
    static bool clone(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, String& err);
    static bool load(SceneObj* scene, Camera2DComponent* camera, const Camera2DInfo& info, const Rect& viewport, String& err);
    static bool load_from_props(SceneObj* scene, ComponentBase** data, const Vector<PropertyValue>& props, String& err);
    static bool unload(SceneObj* scene, ComponentBase** camera, String& err);
    static bool startup(SceneObj* scene, ComponentBase** data, String& err);
    static bool cleanup(SceneObj* scene, ComponentBase** data, String& err);
};

} // namespace LD
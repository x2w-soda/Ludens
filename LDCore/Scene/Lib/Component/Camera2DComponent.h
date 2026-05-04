#pragma once

#include "../SceneObj.h"

namespace LD {

struct Camera2DMeta
{
    static TypeMeta sTypeMeta;

    static void init(ComponentBase** dstData);
    static bool clone(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err);
    static bool load(SceneObj* scene, Camera2DComponent* camera, const Camera2DInfo& info, const Rect& viewport, std::string& err);
    static bool load_from_props(SceneObj* scene, ComponentBase** data, const Vector<PropertyValue>& props, std::string& err);
    static bool unload(SceneObj* scene, ComponentBase** camera, std::string& err);
    static bool startup(SceneObj* scene, ComponentBase** data, std::string& err);
    static bool cleanup(SceneObj* scene, ComponentBase** data, std::string& err);
};

} // namespace LD
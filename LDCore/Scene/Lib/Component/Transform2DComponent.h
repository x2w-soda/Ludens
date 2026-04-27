#pragma once

#include "../SceneObj.h"

namespace LD {

extern struct PropertyMetaTable gTransform2DPropMetaTable;

void init_transform_2d_component(ComponentBase** dstData);
bool clone_transform_2d_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err);

} // namespace LD
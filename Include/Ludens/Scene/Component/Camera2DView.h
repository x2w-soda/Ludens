#pragma once

#include <Ludens/Scene/ComponentView.h>

namespace LD {

struct Camera2DComponent;
struct Camera2DInfo;

/// @brief Public interface for camera 2D components.
class Camera2DView : public ComponentView
{
public:
    Camera2DView() = delete;
    Camera2DView(ComponentView comp);
    Camera2DView(Camera2DComponent* comp);

    bool load(const Camera2DInfo& info, const Rect& viewport, std::string& err);

    Camera2DInfo get_info();
    Rect get_viewport();
    void set_viewport(Rect viewport);

private:
    Camera2DComponent* mCamera = nullptr;
};

} // namespace LD
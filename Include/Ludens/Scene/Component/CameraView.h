#pragma once

#include <Ludens/Scene/ComponentView.h>

namespace LD {

struct CameraComponent;
struct CameraPerspectiveInfo;
struct CameraOrthographicInfo;

/// @brief Public interface for camera components.
class CameraView : public ComponentView
{
public:
    CameraView() = delete;
    CameraView(ComponentView comp);
    CameraView(CameraComponent* comp);

    bool load_perspective(const CameraPerspectiveInfo& info);
    bool load_orthographic(const CameraOrthographicInfo& info);

    bool is_main_camera();
    bool is_perspective();
    bool get_perspective_info(CameraPerspectiveInfo& outInfo);
    bool get_orthographic_info(CameraOrthographicInfo& outInfo);
    void set_perspective(const CameraPerspectiveInfo& info);
    void set_orthographic(const CameraOrthographicInfo& info);

private:
    CameraComponent* mCamera = nullptr;
};

} // namespace LD
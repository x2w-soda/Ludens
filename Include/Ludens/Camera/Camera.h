#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/Header/Math/Vec3.h>

namespace LD {

struct CameraPerspectiveInfo
{
    float fov;         /// field of view in radians
    float aspectRatio; /// width divided by height
    float nearClip;    /// near clip plane
    float farClip;     /// far clip plane
};

struct CameraOrthographicInfo
{
    float left;     /// frustum left plane
    float right;    /// frustum right plane
    float bottom;   /// frustum bottom plane
    float top;      /// frustum top plane
    float nearClip; /// frustum near clip plane
    float farClip;  /// frustum far clip plane
};

/// @brief A camera in 3D space. Each camera is responsible for providing
///        a view matrix and a projection matrix.
struct Camera : Handle<struct CameraObj>
{
    /// @brief create a perspective camera
    /// @param perspectiveInfo description of the camera's perspective projection properties
    /// @param target the position in space that the camera is pointed towards.
    /// @return a camera handle
    static Camera create(const CameraPerspectiveInfo& perspectiveInfo, const Vec3& target);

    /// @brief create a orthographic camera
    /// @param orthographicInfo description of the camera's orthographic projection properties
    /// @param target the position in space that the camera is pointed towards.
    /// @return a camera handle
    static Camera create(const CameraOrthographicInfo& orthographicInfo, const Vec3& target);

    /// @brief destroy a camera
    static void destroy(Camera camera);

    /// @brief get world size from desired screen size
    /// @param worldPos query world position
    /// @param screenSizeY viewport vertical height
    /// @param desiredScreenSizeY screen size to derive world size
    /// @return a world size that at worldPos appears to be desiredScreenSize.
    float screen_to_world_size(const Vec3& worldPos, float screenSizeY, float desiredScreenSizeY) const;

    /// @brief unprojects a position on screen into world space
    /// @param screenPos a position on screen
    /// @param screenSize the screen extent
    /// @param worldNear the world position when unprojected on the near plane
    /// @param worldFar the world position when unprojected on the far plane
    void unproject(const Vec2& screenPos, const Vec2& screenSize, Vec3& worldNear, Vec3& worldFar);

    /// @brief set camera world position
    void set_pos(const Vec3& pos);

    /// @brief set camera target world position
    void set_target(const Vec3& target);

    /// @brief set the up vector, the camera right direction is derived from its forward vector and up vector
    void set_up_vector(const Vec3& up);

    /// @brief configure camera to use perspective projection
    void set_perspective(const CameraPerspectiveInfo& perspectiveInfo);

    /// @brief get the current perspective configuration
    const CameraPerspectiveInfo& get_perspective() const;

    /// @brief configure camera to use orthographic projection
    void set_orthographic(const CameraOrthographicInfo& orthographicInfo);

    /// @brief get the current orthographic configuration
    const CameraOrthographicInfo& get_orthographic() const;

    /// @brief check if camera is using perspective or orthographic projection
    bool is_perspective() const;

    /// @brief get camera world position
    const Vec3& get_pos() const;

    /// @brief get camera target world position
    const Vec3& get_target() const;

    /// @brief get camera view matrix
    const Mat4& get_proj() const;

    /// @brief get camera view matrix
    Mat4 get_view();

    /// @brief get product of camera view matrix and projection matrix
    Mat4 get_view_proj();
};

} // namespace LD
#pragma once

#include <Ludens/Header/Math/Mat3.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/Header/Math/Quat.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Header/Math/Vec3.h>

namespace LD {

/// @brief Transformation in 3D space
struct Transform
{
    Vec3 position; /// world position in 3D space
    Vec3 scale;    /// scale in 3 axis
    Quat rotation; /// rotation as quaternion

    inline Mat4 as_mat4() const
    {
        LD_ASSERT(rotation.is_normalized());
        Mat4 R = rotation.as_mat4();
        return Mat4::translate(position) * R * Mat4::scale(scale);
    }
};

/// @brief Transformation in 3D space, extended for tooling.
struct TransformEx : Transform
{
    Vec3 rotationEuler; /// rotation in 3 axis
};

inline bool decompose_mat4_to_transform(const Mat4& m, TransformEx& transform)
{
    if (!is_equal_epsilon(m[3].w, 1.0f) || !is_zero_epsilon(m[0].w) || !is_zero_epsilon(m[1].w) || !is_zero_epsilon(m[2].w))
        return false;

    Vec3& translation = transform.position;
    Quat& rotation = transform.rotation;
    Vec3& scale = transform.scale;

    translation = Vec3(m[3].x, m[3].y, m[3].z);

    Vec3 x = m[0].as_vec3();
    Vec3 y = m[1].as_vec3();
    Vec3 z = m[2].as_vec3();
    scale.x = x.length();
    scale.y = y.length();
    scale.z = z.length();

    if (is_zero_epsilon(scale.x) || is_zero_epsilon(scale.y) || is_zero_epsilon(scale.z))
        return false;

    x = x / scale.x;
    y = y / scale.y;
    z = z / scale.z;
    Mat3 rotMat(x, y, z);

    if (rotMat.det() < 0.0f)
    {
        scale.x = -scale.x;
        x = -x;
        rotMat = Mat3(x, y, z);
    }

    rotation = Quat::normalize(Quat::from_mat3(rotMat));
    transform.rotationEuler = rotation.as_euler();

    return true;
}

/// @brief Transformation in 2D space
struct Transform2D
{
    Vec2 position;  /// world position in 2D space
    Vec2 scale;     /// scale in 2 axis
    float rotation; /// rotation in degrees

    /// @brief Get transform matrix for homogeneous 2D.
    inline Mat3 as_mat3() const
    {
        return Mat3::translate_2d(position) * Mat3::rotate_2d(LD_TO_RADIANS(rotation)) * Mat3::scale_2d(scale);
    }
};

} // namespace LD
#pragma once

#include <Ludens/Header/Math/Mat3.h>
#include <Ludens/Header/Math/Math.h>
#include <Ludens/Header/Math/Vec3.h>
#include <Ludens/Header/Math/Vec4.h>
#include <Ludens/Header/Platform.h>

// This controls whether the projection matrix should
// perform a handedness flip. The projection matrices of
// GLM performs a flip since its matrices are right-handed
// by default and OpenGL NDC is left-handed. This macro
// decides whether we should follow this quirk.
#define LD_PROJECTION_FLIP_HANDEDNESS 0

namespace LD {

/// @brief 4x4 matrix, containing 4 column vectors
template <typename T>
struct alignas(TVEC4_ALIGNMENT) TMat4
{
    using TVec = TVec4<T>;

    TVec col[4];

    TMat4() : col{} {}
    TMat4(const TVec& c0, const TVec& c1, const TVec& c2, const TVec& c3) : col{c0, c1, c2, c3} {}
    TMat4(T x) : col{TVec(x, 0, 0, 0), TVec(0, x, 0, 0), TVec(0, 0, x, 0), TVec(0, 0, 0, x)} {}
    TMat4(const TMat3<T>& m, T v) : col{TVec(m[0], 0), TVec(m[1], 0), TVec(m[2], 0), TVec(0, 0, 0, v)} {}

    inline TVec& operator[](int i) { return col[i]; }
    inline const TVec& operator[](int i) const { return col[i]; }

    inline T& element(int i) { return col[i / 4][i % 4]; }
    inline T element(int i) const { return col[i / 4][i % 4]; }

    /// @brief return a Mat3 rvalue by dropping the last column and row
    inline TMat3<T> as_mat3() const
    {
        return TMat3<T>(col[0].as_vec3(), col[1].as_vec3(), col[2].as_vec3());
    }

    /// @brief create translation matrix
    static inline TMat4<T> translate(const TVec3<T>& offset)
    {
        TMat4<T> translation((T)1);

        translation[3] = TVec4<T>(offset.x, offset.y, offset.z, (T)1);

        return translation;
    }

    /// @brief create rotation matrix
    /// @param radians rotation angle in radians
    /// @param axis rotation axis, caller ensures this is a unit vector
    static inline TMat4<T> rotate(T radians, const TVec3<T>& axis)
    {
        const T c = LD_COS(radians);
        const T s = LD_SIN(radians);
        const TVec3<T> temp(axis * (static_cast<T>(1) - c));

        TMat4<T> rotation((T)1);

        rotation[0].x = c + temp.x * axis.x;
        rotation[0].y = temp.x * axis.y + s * axis.z;
        rotation[0].z = temp.x * axis.z - s * axis.y;
        rotation[1].x = temp.y * axis.x - s * axis.z;
        rotation[1].y = c + temp.y * axis.y;
        rotation[1].z = temp.y * axis.z + s * axis.x;
        rotation[2].x = temp.z * axis.x + s * axis.y;
        rotation[2].y = temp.z * axis.y - s * axis.x;
        rotation[2].z = c + temp.z * axis.z;

        return rotation;
    }

    /// @brief create scale matrix
    static inline TMat4<T> scale(const TVec3<T>& axis)
    {
        TMat4<T> scale((T)1);

        scale[0].x = axis.x;
        scale[1].y = axis.y;
        scale[2].z = axis.z;

        return scale;
    }

    /// @brief create a view matrix
    /// @param eyePos world space position of the eye
    /// @param targetPos world space position of the target
    /// @param up a directional vector used to derive sideway direction of the eye
    static inline TMat4<T> look_at(const TVec3<T>& eyePos, const TVec3<T>& targetPos, const TVec3<T>& up)
    {
        TVec3<T> eyeDir = TVec3<T>::normalize(targetPos - eyePos);
        TVec3<T> eyeSide = TVec3<T>::normalize(TVec3<T>::cross(eyeDir, up));
        TVec3<T> eyeUp = TVec3<T>::cross(eyeSide, eyeDir);

        TMat4 view(1);
        view[0].x = eyeSide.x;
        view[1].x = eyeSide.y;
        view[2].x = eyeSide.z;
        view[0].y = eyeUp.x;
        view[1].y = eyeUp.y;
        view[2].y = eyeUp.z;
        view[0].z = -eyeDir.x;
        view[1].z = -eyeDir.y;
        view[2].z = -eyeDir.z;
        view[3].x = -TVec3<T>::dot(eyeSide, eyePos);
        view[3].y = -TVec3<T>::dot(eyeUp, eyePos);
        view[3].z = TVec3<T>::dot(eyeDir, eyePos);

        return view;
    }

    /// @brief create a perspective projection matrix
    static inline TMat4<T> perspective(T fovRadians, T aspect, T clipNear, T clipFar)
    {
        T tanFov2 = LD_TAN(fovRadians / static_cast<T>(2));

        TMat4 proj((T)0);
        proj[0].x = (T)1 / (aspect * tanFov2);
        proj[1].y = (LD_PROJECTION_FLIP_HANDEDNESS ? (T)1 : (T)-1) / (tanFov2);
        proj[2].z = -(clipFar + clipNear) / (clipFar - clipNear);
        proj[2].w = -(T)1;
        proj[3].z = -((T)2 * clipFar * clipNear) / (clipFar - clipNear);

        return proj;
    }

    /// @brief create a orthographic projection matrix
    static inline TMat4<T> orthographic(T left, T right, T bottom, T top, T clipNear, T clipFar)
    {
        if constexpr (LD_PROJECTION_FLIP_HANDEDNESS)
        {
            T tmp = top;
            top = bottom;
            bottom = tmp;
        }

        TMat4 proj((T)1);
        proj[0].x = (T)2 / (right - left);
        proj[1].y = (T)2 / (bottom - top);
        proj[2].z = (T)-2 / (clipFar - clipNear);
        proj[3].x = -(right + left) / (right - left);
        proj[3].y = -(top + bottom) / (bottom - top);
        proj[3].z = -(clipFar + clipNear) / (clipFar - clipNear);

        return proj;
    }

    /// @brief create an orthographic projection matrix from screen extent
    static inline TMat4<T> orthographic_extent(const TVec2<T>& extent)
    {
        return TMat4<T>::orthographic((T)0, extent.x, extent.y, (T)0, (T)-1, (T)1);
    }

    /// @brief create an inverse matrix
    /// @warning does not check if matrix is reversible, crashes upon zero determinant
    static inline TMat4 inverse(const TMat4& m)
    {
        TMat4 inv;

#define M(R, C) m[C].R
        inv[0].x = +TMat3<T>(TVec3<T>(M(y, 1), M(z, 1), M(w, 1)), TVec3<T>(M(y, 2), M(z, 2), M(w, 2)), TVec3<T>(M(y, 3), M(z, 3), M(w, 3))).det();
        inv[0].y = -TMat3<T>(TVec3<T>(M(y, 0), M(z, 0), M(w, 0)), TVec3<T>(M(y, 2), M(z, 2), M(w, 2)), TVec3<T>(M(y, 3), M(z, 3), M(w, 3))).det();
        inv[0].z = +TMat3<T>(TVec3<T>(M(y, 0), M(z, 0), M(w, 0)), TVec3<T>(M(y, 1), M(z, 1), M(w, 1)), TVec3<T>(M(y, 3), M(z, 3), M(w, 3))).det();
        inv[0].w = -TMat3<T>(TVec3<T>(M(y, 0), M(z, 0), M(w, 0)), TVec3<T>(M(y, 1), M(z, 1), M(w, 1)), TVec3<T>(M(y, 2), M(z, 2), M(w, 2))).det();

        inv[1].x = -TMat3<T>(TVec3<T>(M(x, 1), M(z, 1), M(w, 1)), TVec3<T>(M(x, 2), M(z, 2), M(w, 2)), TVec3<T>(M(x, 3), M(z, 3), M(w, 3))).det();
        inv[1].y = +TMat3<T>(TVec3<T>(M(x, 0), M(z, 0), M(w, 0)), TVec3<T>(M(x, 2), M(z, 2), M(w, 2)), TVec3<T>(M(x, 3), M(z, 3), M(w, 3))).det();
        inv[1].z = -TMat3<T>(TVec3<T>(M(x, 0), M(z, 0), M(w, 0)), TVec3<T>(M(x, 1), M(z, 1), M(w, 1)), TVec3<T>(M(x, 3), M(z, 3), M(w, 3))).det();
        inv[1].w = +TMat3<T>(TVec3<T>(M(x, 0), M(z, 0), M(w, 0)), TVec3<T>(M(x, 1), M(z, 1), M(w, 1)), TVec3<T>(M(x, 2), M(z, 2), M(w, 2))).det();

        inv[2].x = +TMat3<T>(TVec3<T>(M(x, 1), M(y, 1), M(w, 1)), TVec3<T>(M(x, 2), M(y, 2), M(w, 2)), TVec3<T>(M(x, 3), M(y, 3), M(w, 3))).det();
        inv[2].y = -TMat3<T>(TVec3<T>(M(x, 0), M(y, 0), M(w, 0)), TVec3<T>(M(x, 2), M(y, 2), M(w, 2)), TVec3<T>(M(x, 3), M(y, 3), M(w, 3))).det();
        inv[2].z = +TMat3<T>(TVec3<T>(M(x, 0), M(y, 0), M(w, 0)), TVec3<T>(M(x, 1), M(y, 1), M(w, 1)), TVec3<T>(M(x, 3), M(y, 3), M(w, 3))).det();
        inv[2].w = -TMat3<T>(TVec3<T>(M(x, 0), M(y, 0), M(w, 0)), TVec3<T>(M(x, 1), M(y, 1), M(w, 1)), TVec3<T>(M(x, 2), M(y, 2), M(w, 2))).det();

        inv[3].x = -TMat3<T>(TVec3<T>(M(x, 1), M(y, 1), M(z, 1)), TVec3<T>(M(x, 2), M(y, 2), M(z, 2)), TVec3<T>(M(x, 3), M(y, 3), M(z, 3))).det();
        inv[3].y = +TMat3<T>(TVec3<T>(M(x, 0), M(y, 0), M(z, 0)), TVec3<T>(M(x, 2), M(y, 2), M(z, 2)), TVec3<T>(M(x, 3), M(y, 3), M(z, 3))).det();
        inv[3].z = -TMat3<T>(TVec3<T>(M(x, 0), M(y, 0), M(z, 0)), TVec3<T>(M(x, 1), M(y, 1), M(z, 1)), TVec3<T>(M(x, 3), M(y, 3), M(z, 3))).det();
        inv[3].w = +TMat3<T>(TVec3<T>(M(x, 0), M(y, 0), M(z, 0)), TVec3<T>(M(x, 1), M(y, 1), M(z, 1)), TVec3<T>(M(x, 2), M(y, 2), M(z, 2))).det();
#undef M

        T det = m.col[0].x * inv.col[0].x + m.col[0].y * inv.col[1].x + m.col[0].z * inv.col[2].x + m.col[0].w * inv.col[3].x;

        if (is_zero_epsilon<T>(det))
        {
            LD_UNREACHABLE; // crash in debug
            return TMat4(1);
        }

        T invDet = static_cast<T>(1) / det;

        for (int i = 0; i < 4; i++)
            inv.col[i] = inv.col[i] * invDet;

        return inv;
    }
};

template <typename T>
inline TVec4<T> operator*(const TMat4<T>& m, const TVec4<T>& v)
{
    return m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3] * v.w;
}

/// @brief mat4 multiplication, rhs is applied before lhs
template <typename T>
inline TMat4<T> operator*(const TMat4<T>& lhs, const TMat4<T>& rhs)
{
    return TMat4<T>(lhs * rhs[0], lhs * rhs[1], lhs * rhs[2], lhs * rhs[3]);
}

using Mat4 = TMat4<float>;
using IMat4 = TMat4<int>;
using DMat4 = TMat4<double>;

} // namespace LD
#pragma once

#include <Ludens/Gizmo/GizmoEnum.h>
#include <Ludens/Header/Math/Transform.h>

namespace LD {
namespace Gizmo2D {

/// @brief 2D translation gizmo.
/// @param childLocal Child local TRS.
/// @param parentWorld Parent world TRS.
/// @param mouseWorldPos Mouse world position now.
/// @param startMouseOffset Mouse world position when gizmo started.
/// @return New child local TRS.
Transform2D translate(const Transform2D& childLocal, const Transform2D& parentWorld, Vec2 mouseWorldPos, Vec2 startMouseOffset);

/// @brief 2D rotation gizmo.
/// @param childLocal Child local TRS.
/// @param parentWorld Parent world TRS.
/// @param mouseWorldPos Mouse world position now.
/// @param startMouseOffsetDeg Degrees between mouse world position and child world position when gizmo started.
/// @param startChildWorldDeg Child world rotation in degrees when gizmo started.
/// @return New child local TRS.
Transform2D rotate(const Transform2D& childLocal, const Transform2D& parentWorld, Vec2 mouseWorldPos, float startMouseOffsetDeg, float startChildWorldDeg);

/// @brief 2D scale gizmo.
/// @param childLocal Child local TRS.
/// @param parentWorld Parent world TRS.
/// @param mouseWorldPos Mouse world position now.
/// @param startDist Distance between mouse world position and child world position when gizmo started.
/// @param startChildWorldScale Child world scale when gizmo started.
/// @return New child local TRS.
Transform2D scale(const Transform2D& childLocal, const Transform2D& parentWorld, Vec2 mouseWorldPos, float startDist, Vec2 startChildWorldScale);

} // namespace Gizmo2D
} // namespace LD
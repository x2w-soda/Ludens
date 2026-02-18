#pragma once

#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/RenderComponent/SceneOverlayComponent.h>
#include <Ludens/Serial/SUID.h>

#define VIEWPORT_TOOLBAR_HEIGHT 26.0f

namespace LD {

struct ViewportState
{
    Vec2 viewportExtent;              /// width and height of the entire viewport window
    Vec2 sceneExtent;                 /// width and height of the scene inside the viewport
    Vec2 sceneMousePos;               /// mouse position in sceneExtent
    float delta;                      /// frame delta time in seconds
    RUID hoverRUID;                   /// the render ID under mouse cursor
    SUID gizmoSubjectSUID;            /// component subject to gizmo controls
    SceneOverlayGizmo gizmoType;      /// current gizmo control mode
    SceneOverlayGizmoID hoverGizmoID; /// the gizmo mesh under mouse cursor
};

} // namespace LD
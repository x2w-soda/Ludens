#pragma once

#include <Ludens/Media/Model.h>
#include <Ludens/RenderBackend/RBackend.h>

namespace LD {

struct EmbeddedGizmoMesh
{
    static void get_translation_gizmo_axis(const MeshVertex** vertices, uint32_t& vertexCount, const uint32_t** indices, uint32_t& indexCount);
    static void get_translation_gizmo_axis_draw_info(RDrawIndexedInfo& drawInfo);

    static void get_scale_gizmo_axis(const MeshVertex** vertices, uint32_t& vertexCount, const uint32_t** indices, uint32_t& indexCount);
    static void get_scale_gizmo_axis_draw_info(RDrawIndexedInfo& drawInfo);

    static void get_gizmo_plane_xy(const MeshVertex** vertices, uint32_t& vertexCount);
    static void get_gizmo_plane_xz(const MeshVertex** vertices, uint32_t& vertexCount);
    static void get_gizmo_plane_yz(const MeshVertex** vertices, uint32_t& vertexCount);
    static void get_gizmo_plane_draw_info(RDrawInfo& drawInfo);
};

} // namespace LD
#pragma once

#include <Ludens/Media/Model.h>
#include <Ludens/RenderBackend/RBackend.h>

namespace LD {

struct EmbeddedGizmoMesh
{
    static void get_translation_gizmo_x(const MeshVertex** vertices, uint32_t& vertexCount, const uint32_t** indices, uint32_t& indexCount);
    static void get_translation_gizmo_x_draw(RDrawIndexedInfo& drawInfo);

    static void get_scale_gizmo_x(const MeshVertex** vertices, uint32_t& vertexCount, const uint32_t** indices, uint32_t& indexCount);
    static void get_scale_gizmo_x_draw(RDrawIndexedInfo& drawInfo);
};

} // namespace LD
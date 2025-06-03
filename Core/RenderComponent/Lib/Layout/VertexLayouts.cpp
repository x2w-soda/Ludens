#include <Ludens/RenderComponent/Layout/VertexLayouts.h>
#include <algorithm>

namespace LD {

// clang-format off
const Vec3 sCubePos[36] = {
    {-1.0f, +1.0f, -1.0f},
    {-1.0f, -1.0f, -1.0f},
    {+1.0f, -1.0f, -1.0f},
    {+1.0f, -1.0f, -1.0f},
    {+1.0f, +1.0f, -1.0f},
    {-1.0f, +1.0f, -1.0f},

    {-1.0f, -1.0f, +1.0f},
    {-1.0f, -1.0f, -1.0f},
    {-1.0f, +1.0f, -1.0f},
    {-1.0f, +1.0f, -1.0f},
    {-1.0f, +1.0f, +1.0f},
    {-1.0f, -1.0f, +1.0f},

    {+1.0f, -1.0f, -1.0f},
    {+1.0f, -1.0f, +1.0f},
    {+1.0f, +1.0f, +1.0f},
    {+1.0f, +1.0f, +1.0f},
    {+1.0f, +1.0f, -1.0f},
    {+1.0f, -1.0f, -1.0f},

    {-1.0f, -1.0f, +1.0f},
    {-1.0f, +1.0f, +1.0f},
    {+1.0f, +1.0f, +1.0f},
    {+1.0f, +1.0f, +1.0f},
    {+1.0f, -1.0f, +1.0f},
    {-1.0f, -1.0f, +1.0f},

    {-1.0f, +1.0f, -1.0f},
    {+1.0f, +1.0f, -1.0f},
    {+1.0f, +1.0f, +1.0f},
    {+1.0f, +1.0f, +1.0f},
    {-1.0f, +1.0f, +1.0f},
    {-1.0f, +1.0f, -1.0f},

    {-1.0f, -1.0f, -1.0f},
    {-1.0f, -1.0f, +1.0f},
    {+1.0f, -1.0f, -1.0f},
    {+1.0f, -1.0f, -1.0f},
    {-1.0f, -1.0f, +1.0f},
    {+1.0f, -1.0f, +1.0f},
};
// clang-format on

void get_cube_mesh_vertex_attributes(Vec3* pos)
{
    if (pos)
    {
        std::copy(sCubePos, sCubePos + 36, pos);
    }
}

} // namespace LD
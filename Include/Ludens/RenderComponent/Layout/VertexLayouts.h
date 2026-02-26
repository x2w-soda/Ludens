#pragma once

#include <Ludens/Media/Model.h>
#include <Ludens/RenderBackend/RBackend.h>

#include <algorithm>
#include <cstdint>

namespace LD {

static inline void get_mesh_vertex_attributes(std::vector<RVertexAttribute>& attrs)
{
    attrs.resize(3);
    attrs[0] = {GLSL_TYPE_VEC3, sizeof(float) * 0, 0}; // offset
    attrs[1] = {GLSL_TYPE_VEC3, sizeof(float) * 3, 0}; // normal
    attrs[2] = {GLSL_TYPE_VEC2, sizeof(float) * 6, 0}; // uv

    static_assert(sizeof(MeshVertex) == 32);
}

/// @brief Four quad vertices form a quad primitive, which is used to render images, font glyphs, rects, etc.
struct QuadVertex
{
    float x, y;
    float u, v;
    uint32_t color;
    uint32_t control;
};

static inline void get_quad_vertex_attributes(std::vector<RVertexAttribute>& attr)
{
    attr.resize(4);
    attr[0].type = GLSL_TYPE_VEC2; // position
    attr[0].binding = 0;
    attr[0].offset = offsetof(QuadVertex, x);
    attr[1].type = GLSL_TYPE_VEC2; // uv
    attr[1].binding = 0;
    attr[1].offset = offsetof(QuadVertex, u);
    attr[2].type = GLSL_TYPE_UINT; // color
    attr[2].binding = 0;
    attr[2].offset = offsetof(QuadVertex, color);
    attr[3].type = GLSL_TYPE_UINT; // control
    attr[3].binding = 0;
    attr[3].offset = offsetof(QuadVertex, control);

    static_assert(sizeof(QuadVertex) == 24);
}

enum QuadMode
{
    QUAD_MODE_NONE = 0,            /// regular sampling
    QUAD_MODE_FONT = 1,            /// sampling a single channel bitmap atlas
    QUAD_MODE_FONT_SDF = 2,        /// sampling a single channel signed distanced field
    QUAD_MODE_FORCE_ALPHA_ONE = 3, /// use 1.0 for alpha instead of sampled alpha channel
    QUAD_MODE_ELLIPSE = 4,         /// uses local UV signed distance to determine ellipse area
    QUAD_MODE_RECT_ROUNDED = 5,    /// ratio1 aspect ratio, r2 border ratio. draw with border radius
};

/// @brief Get QuadVertex control bits. [0:1] localUV, [2:5] imageIdx, [6:9] quadMode, [10:19] ratio1 [20:29] ratio2
/// @param imageIdx 4 bits to encode the image index in array
/// @param mode 4 bits to hint how the quad should be behave in fragment shader
/// @param ratio1 10 bits for a normalized ratio
/// @param ratio2 10 bits for a normalized ratio
/// @return control bits for the four quad vertices, they only differ in the first 2 bits.
static inline UVec4 get_quad_vertex_control_bits(int imageIdx, QuadMode mode, float ratio1, float ratio2)
{
    uint32_t controlBits = 0;
    uint32_t modeBits = static_cast<uint32_t>(mode) & 15;
    uint32_t ratio1Bits = static_cast<uint32_t>(ratio1 * 1023.0f);
    uint32_t ratio2Bits = static_cast<uint32_t>(ratio2 * 1023.0f);

    controlBits |= ((imageIdx & 15) << 2);
    controlBits |= (modeBits << 6);
    controlBits |= ((ratio1Bits & 1023) << 10);
    controlBits |= ((ratio2Bits & 1023) << 20);

    UVec4 quadControls;
    quadControls.x = controlBits;     // TL control, local UV (0, 0)
    quadControls.y = controlBits | 1; // TR control, local UV (1, 0)
    quadControls.z = controlBits | 3; // BR control, local UV (1, 1)
    quadControls.w = controlBits | 2; // BL control, local UV (0, 1)

    return quadControls;
}

/// @brief Helper to accumulate QuadVertex data on the CPU side
template <uint32_t TMaxQuadCount>
class QuadVertexBatch
{
public:
    /// @brief append a quad to the batch
    /// @return A pointer to 4 QuadVertices, describing a quad.
    /// @warning does not check if the batch is full
    QuadVertex* write_quad()
    {
        return (QuadVertex*)mVertices + mQuadCount++ * 4;
    }

    /// @brief get number of rects in the batch
    uint32_t get_quad_count() const
    {
        return mQuadCount;
    };

    /// @brief get maximum number of rects in the batch
    constexpr uint32_t get_max_rect_count() const
    {
        return TMaxQuadCount;
    }

    /// @brief get the rect vertices in the batch
    QuadVertex* get_vertices(uint32_t& vertexCount)
    {
        vertexCount = mQuadCount * 4;
        return mVertices;
    }

    /// @brief reset the batch
    void reset()
    {
        mQuadCount = 0;
    }

    /// @brief return whether the batch is full
    bool is_full() const
    {
        return mQuadCount >= TMaxQuadCount;
    }

    // write index pattern for the full index buffer
    void write_indices(uint32_t* indices)
    {
        for (uint32_t i = 0; i < TMaxQuadCount; i++)
        {
            indices[6 * i + 0] = 4 * i + 0;
            indices[6 * i + 1] = 4 * i + 1;
            indices[6 * i + 2] = 4 * i + 2;
            indices[6 * i + 3] = 4 * i + 2;
            indices[6 * i + 4] = 4 * i + 3;
            indices[6 * i + 5] = 4 * i + 0;
        }
    }

private:
    uint32_t mQuadCount = 0;
    QuadVertex mVertices[TMaxQuadCount * 4];
};

struct PointVertex
{
    float x, y, z;
    uint32_t color;
};

static inline void get_point_vertex_attributes(std::vector<RVertexAttribute>& attr)
{
    attr.resize(2);
    attr[0].type = GLSL_TYPE_VEC3;
    attr[0].offset = offsetof(PointVertex, x);
    attr[0].binding = 0;
    attr[1].type = GLSL_TYPE_UINT;
    attr[1].offset = offsetof(PointVertex, color);
    attr[1].binding = 0;
}

/// @brief helper to accumulate PointVertex data on the CPU side
template <uint32_t TMaxPointCount>
class PointVertexBatch
{
public:
    /// @brief reset the batch
    void reset()
    {
        mPointCount = 0;
    }

    /// @brief append a line to the batch by writing 2 PointVertices
    /// @warning does not check if the batch is full
    void write_line(const Vec3& p0, const Vec3& p1, uint32_t color)
    {
        mVertices[mPointCount].x = p0.x;
        mVertices[mPointCount].y = p0.y;
        mVertices[mPointCount].z = p0.z;
        mVertices[mPointCount].color = color;
        mVertices[mPointCount + 1].x = p1.x;
        mVertices[mPointCount + 1].y = p1.y;
        mVertices[mPointCount + 1].z = p1.z;
        mVertices[mPointCount + 1].color = color;
        mPointCount += 2;
    }

    /// @brief get number of PointVertices in the batch
    uint32_t get_point_count() const
    {
        return mPointCount;
    }

    /// @brief get maximum PointVertex capacity
    constexpr uint32_t get_point_capacity() const
    {
        return TMaxPointCount;
    }

    /// @brief get the rect vertices in the batch
    PointVertex* get_vertices(uint32_t& vertexCount)
    {
        vertexCount = mPointCount;
        return mVertices;
    }

private:
    uint32_t mPointCount = 0;
    PointVertex mVertices[TMaxPointCount];
};

/// @brief get mesh vertex attributes for a unit cube.
/// @param pos if not null, an array of 36 Vec3 that will be written to.
void get_cube_mesh_vertex_attributes(Vec3* pos);

} // namespace LD

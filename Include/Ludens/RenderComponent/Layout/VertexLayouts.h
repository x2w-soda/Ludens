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

struct RectVertex
{
    float x, y;
    float u, v;
    uint32_t color;
    uint32_t control;
};

static inline void get_rect_vertex_attributes(std::vector<RVertexAttribute>& attr)
{
    attr.resize(4);
    attr[0].type = GLSL_TYPE_VEC2; // position
    attr[0].binding = 0;
    attr[0].offset = offsetof(RectVertex, x);
    attr[1].type = GLSL_TYPE_VEC2; // uv
    attr[1].binding = 0;
    attr[1].offset = offsetof(RectVertex, u);
    attr[2].type = GLSL_TYPE_UINT; // color
    attr[2].binding = 0;
    attr[2].offset = offsetof(RectVertex, color);
    attr[3].type = GLSL_TYPE_UINT; // control
    attr[3].binding = 0;
    attr[3].offset = offsetof(RectVertex, control);

    static_assert(sizeof(RectVertex) == 24);
}

enum RectVertexImageHint
{
    RECT_VERTEX_IMAGE_HINT_NONE = 0,      /// regular bitmap image
    RECT_VERTEX_IMAGE_HINT_FONT = 1,      /// single channel bitmap atlas
    RECT_VERTEX_IMAGE_HINT_FONT_SDF = 2,  /// single channel signed distanced field
    RECT_VERTEX_IMAGE_HINT_ALPHA_ONE = 3, /// use 1.0 for alpha instead of image alpha channel
};

/// @brief get rect vertex control bits. [0:3] imageIdx, [4:7] imageHint, [8:15] filter ratio
/// @param imageIdx 4 bits to encode the image index in array
/// @param imageHint 4 bits to hint how the image should be used
/// @param filterRatio 8 bits to encode the filtering ratio used for SDF font rendering. A ratio from 0.0 to 32.0 can be represented at a step of 0.125
/// @return control bits for a RectVertex
static inline uint32_t get_rect_vertex_control_bits(int imageIdx, RectVertexImageHint imageHint, float filterRatio)
{
    uint32_t controlBits = 0;
    uint32_t imageHintBits = static_cast<uint32_t>(imageHint) & 15;
    uint32_t filterRatioBits = std::clamp<uint32_t>(static_cast<uint32_t>(filterRatio * 8.0f + 0.5f), 0, 255);

    controlBits |= (imageIdx & 15);
    controlBits |= (imageHintBits << 4);
    controlBits |= (filterRatioBits << 8);

    return controlBits;
}

/// @brief helper to accumulate RectVertex data on the CPU side
template <uint32_t TMaxRectCount>
class RectVertexBatch
{
public:
    /// @brief append a rect to the batch
    /// @return A pointer to 4 RectVertices, describing a rect.
    /// @warning does not check if the batch is full
    RectVertex* write_rect()
    {
        return (RectVertex*)mVertices + mRectCount++ * 4;
    }

    /// @brief get number of rects in the batch
    uint32_t get_rect_count() const
    {
        return mRectCount;
    };

    /// @brief get maximum number of rects in the batch
    constexpr uint32_t get_max_rect_count() const
    {
        return TMaxRectCount;
    }

    /// @brief get the rect vertices in the batch
    RectVertex* get_vertices(uint32_t& vertexCount)
    {
        vertexCount = mRectCount * 4;
        return mVertices;
    }

    /// @brief reset the batch
    void reset()
    {
        mRectCount = 0;
    }

    /// @brief return whether the batch is full
    bool is_full() const
    {
        return mRectCount >= TMaxRectCount;
    }

    // write index pattern for the full index buffer
    void write_indices(uint32_t* indices)
    {
        for (uint32_t i = 0; i < TMaxRectCount; i++)
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
    uint32_t mRectCount = 0;
    RectVertex mVertices[TMaxRectCount * 4];
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

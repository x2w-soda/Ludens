#pragma once

#include <Ludens/Media/Model.h>

namespace LD {

static inline void get_mesh_vertex_attributes(std::vector<RVertexAttribute>& attrs)
{
    attrs.resize(3);
    attrs[0] = {RGLSL_TYPE_VEC3, sizeof(float) * 0, 0}; // offset
    attrs[1] = {RGLSL_TYPE_VEC3, sizeof(float) * 3, 0}; // normal
    attrs[2] = {RGLSL_TYPE_VEC2, sizeof(float) * 6, 0}; // uv

    static_assert(sizeof(MeshVertex) == 32);
}

struct RectVertex
{
    float x, y;
    float u, v;
    uint32_t color;
    uint32_t imageIdx;
};

static inline void get_rect_vertex_attributes(std::vector<RVertexAttribute>& attr)
{
    attr.resize(4);
    attr[0].type = RGLSL_TYPE_VEC2; // position
    attr[0].binding = 0;
    attr[0].offset = offsetof(RectVertex, x);
    attr[1].type = RGLSL_TYPE_VEC2; // uv
    attr[1].binding = 0;
    attr[1].offset = offsetof(RectVertex, u);
    attr[2].type = RGLSL_TYPE_UINT; // color
    attr[2].binding = 0;
    attr[2].offset = offsetof(RectVertex, color);
    attr[3].type = RGLSL_TYPE_UINT; // image index
    attr[3].binding = 0;
    attr[3].offset = offsetof(RectVertex, imageIdx);

    static_assert(sizeof(RectVertex) == 24);
}

/// @brief helper to accumulate RectVertex data on the CPU side
template <typename uint32_t TMaxRectCount>
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
    attr[0].type = RGLSL_TYPE_VEC3;
    attr[0].offset = offsetof(PointVertex, x);
    attr[0].binding = 0;
    attr[1].type = RGLSL_TYPE_UINT;
    attr[1].offset = offsetof(PointVertex, color);
    attr[1].binding = 0;
}

/// @brief helper to accumulate PointVertex data on the CPU side
template <typename uint32_t TMaxPointCount>
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

} // namespace LD
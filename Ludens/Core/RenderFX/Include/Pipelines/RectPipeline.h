#pragma once

#include <functional>
#include "Core/DSA/Include/Optional.h"
#include "Core/Header/Include/Types.h"
#include "Core/Math/Include/Mat4.h"
#include "Core/Math/Include/Rect2D.h"
#include "Core/Math/Include/Vec4.h"
#include "Core/OS/Include/Memory.h"
#include "Core/RenderBase/Include/RBuffer.h"
#include "Core/RenderBase/Include/RDevice.h"
#include "Core/RenderBase/Include/RPipeline.h"
#include "Core/RenderBase/Include/RShader.h"
#include "Core/RenderFX/Include/RBatch.h"
#include "Core/RenderFX/Include/PrefabPipeline.h"
#include "Core/Media/Include/Font.h"

namespace LD
{

struct RectVertex
{
    Vec2 Position;
    Vec2 TexUV;
    Vec4 Color;
    float TexID;
};

/// rect batching utility to construct vertex and index buffers,
/// which can be drawn using the rect pipeline.
class RectBatch
{
public:
    RectBatch();
    RectBatch(const RectBatch&) = delete;
    ~RectBatch();

    RectBatch& operator=(const RectBatch&) = delete;

    void Startup(RDevice device, int rectCapacity);
    void Cleanup();

    inline bool IsFull() const
    {
        return mBatch.GetElementCapacity() == mBatch.GetElementCount();
    }

    void Reset();
    bool AddCustom(const RectVertex* vertices);
    bool AddRectOutline(const Rect2D& rect, Vec4 color, float lineWidth);
    bool AddRectFilled(const Rect2D& rect, Vec4 color);
    bool AddTexture(const Rect2D& rect, const Rect2D& texRegion, Vec2 texSize, Vec4 color, int texID);

    /// @brief add a font glyph
    /// @param cursor the cursor resides on the baseline
    /// @param glyph the glyph to render, whose bounding box is decided from cursor and glyph bearing.
    /// @param scale the scale applied to glyph size, bearing, and font metrics
    /// @param color glyph color
    /// @param texID the expected binding index of the font atlas
    bool AddGlyph(const Vec2& cursor, const FontGlyph& glyph, float scale, Vec4 color, int texID);

    int GetRectCount();
    int GetRectCommitedCount();
    void Commit();
    void GetBuffers(RBuffer& vertexBuffer, RBuffer& indexBuffer);

private:
    RDevice mDevice;
    RBuffer mVertexBuffer;
    RBuffer mIndexBuffer;
    RBatch<RectVertex, u16> mBatch;
    int mRectCommited;
};

/// Manages one or more RectBatches, such that there is always space to add new elements.
/// User supplies a OnCommit callback that is called whenever a RectBatch is full,
/// or when RectBatcher::Commit is called.
class RectBatcher
{
public:

    /// called by the batcher whenever a batch is full
    using OnCommit = std::function<void(RBuffer vertexBuffer, RBuffer indexBuffer, int indexStart, int indexCount)>;

    RectBatcher();
    RectBatcher(const RectBatcher&) = delete;
    ~RectBatcher();

    RectBatcher& operator=(const RectBatcher&) = delete;

    void Startup(RDevice device, int rectCapacity, OnCommit callback);
    void Cleanup();

    /// reset all batches
    void Reset();

    /// manually perform a commit
    void Commit();

    void AddCustom(const RectVertex* vertices);
    void AddRectOutline(const Rect2D& rect, Vec4 color, float lineWidth);
    void AddRectFilled(const Rect2D& rect, Vec4 color);
    void AddTexture(const Rect2D& rect, const Rect2D& texRegion, Vec2 texSize, Vec4 color, int texID);
    void AddGlyph(const Vec2& cursor, const FontGlyph& glyph, float scale, Vec4 color, int texID);

private:
    RectBatch* GetRectBatch(int reserve);

    RDevice mDevice;
    Vector<RectBatch*> mBatches;
    OnCommit mCommitCallback;
    int mBatchCapacity;
    int mBatchCtr;
};

struct RectPipelineInfo
{
    RDevice Device;
    RPipelineLayout RectPipelineLayout;
    RPass RenderPass;
};

class RectPipeline : public PrefabPipeline
{
public:
    RectPipeline();
    RectPipeline(const RectPipeline&) = delete;
    ~RectPipeline();

    RectPipeline& operator=(const RectPipeline&) = delete;

    void Startup(const RectPipelineInfo& info);
    void Cleanup();

    virtual RPipelineLayoutData GetLayoutData() const override;

private:
    RDevice mDevice;
    RShader mRectVS;
    RShader mRectFS;
};

} // namespace LD
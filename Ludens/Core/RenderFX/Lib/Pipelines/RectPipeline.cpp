#include <unordered_map>
#include "Core/Header/Include/Error.h"
#include "Core/Math/Include/Mat4.h"
#include "Core/Math/Include/Math.h"
#include "Core/DSA/Include/Array.h"
#include "Core/RenderBase/Include/RBinding.h"
#include "Core/RenderBase/Include/RTexture.h"
#include "Core/RenderBase/Include/RBuffer.h"
#include "Core/RenderBase/Include/RPipeline.h"
#include "Core/RenderBase/Include/RShader.h"
#include "Core/RenderFX/Include/Pipelines/RectPipeline.h"
#include "Core/RenderFX/Include/Groups/ViewportGroup.h"
#include "Core/RenderFX/Include/Groups/RectGroup.h"
#include "Core/Media/Include/Image.h"

namespace LD {

namespace Embed {

extern void GetRectGLVS(unsigned int* size, const char** data);
extern void GetRectGLFS(unsigned int* size, const char** data);
extern void GetRectVKVS(unsigned int* size, const char** data);
extern void GetRectVKFS(unsigned int* size, const char** data);

} // namespace Embed

static const Vec2 sUnitRectPos[] = {
    { 0.5f, 0.5f },
    { 0.5f, -0.5f },
    { -0.5f, -0.5f },
    { -0.5f, 0.5f },
};

RectBatch::RectBatch()
{
}

RectBatch::~RectBatch()
{
    LD_DEBUG_ASSERT(!mDevice);
}

void RectBatch::Startup(RDevice device, int capacity)
{
    LD_DEBUG_ASSERT(capacity >= 4 && "AddRectOutline uses 4 rects");
    LD_DEBUG_ASSERT(capacity < 65535 / 6 && "16 bit indices");

    // one rect is 4 vertices referenced by 6 indices
    int indexSequence[] = { 0, 1, 2, 2, 3, 0 };

    mDevice = device;
    mBatch.Startup(4, { 6, indexSequence }, capacity);

    RBufferInfo info{};
    info.MemoryUsage = RMemoryUsage::FrameDynamic;
    info.Type = RBufferType::VertexBuffer;
    info.Size = mBatch.GetVertexBufferSize();
    info.Data = nullptr;
    mDevice.CreateBuffer(mVertexBuffer, info);

    info.MemoryUsage = RMemoryUsage::Immutable;
    info.Type = RBufferType::IndexBuffer;
    info.Size = mBatch.GetIndexBufferSize();
    info.Data = mBatch.GetIndices();
    mDevice.CreateBuffer(mIndexBuffer, info);

    mRectCommited = 0;
}

void RectBatch::Cleanup()
{
    mDevice.DeleteBuffer(mIndexBuffer);
    mDevice.DeleteBuffer(mVertexBuffer);
    mBatch.Cleanup();
    mDevice.ResetHandle();
}

void RectBatch::Reset()
{
    mBatch.Reset();
    mRectCommited = 0;
}

bool RectBatch::AddCustom(const RectVertex* vertices)
{
    if (mBatch.GetElementCapacity() == mBatch.GetElementCount())
        return false;

    bool ok = mBatch.AddElement(vertices);
    LD_DEBUG_ASSERT(ok);
    return ok;
}

bool RectBatch::AddRectOutline(const Rect2D& rect, Vec4 color, float lineWidth)
{
    if (mBatch.GetElementCapacity() - mBatch.GetElementCount() < 4)
        return false;

    Rect2D borderL{ rect.x, rect.y, lineWidth, rect.h };
    Rect2D borderR{ rect.x + rect.w - lineWidth, rect.y, lineWidth, rect.h };
    Rect2D borderT{ rect.x, rect.y, rect.w, lineWidth };
    Rect2D borderB{ rect.x, rect.y + rect.h - lineWidth, rect.w, lineWidth };

    bool ok = AddRectFilled(borderL, color) && AddRectFilled(borderR, color) && AddRectFilled(borderT, color) &&
              AddRectFilled(borderB, color);
    LD_DEBUG_ASSERT(ok);
    return ok;
}

bool RectBatch::AddRectFilled(const Rect2D& rect, Vec4 color)
{
    RectVertex vertex[4];

    if (mBatch.GetElementCapacity() == mBatch.GetElementCount())
        return false;

    for (int i = 0; i < 4; i++)
    {
        float x = sUnitRectPos[i].x + 0.5f;
        float y = sUnitRectPos[i].y + 0.5f;

        vertex[i].Color = color;
        vertex[i].Position.x = (x * rect.w) + rect.x;
        vertex[i].Position.y = (y * rect.h) + rect.y;
        vertex[i].TexID = 0.0f; // white pixel texture
    }

    vertex[0].TexUV = { 1.0f, 0.0f };
    vertex[1].TexUV = { 1.0f, 1.0f };
    vertex[2].TexUV = { 0.0f, 1.0f };
    vertex[3].TexUV = { 0.0f, 0.0f };

    bool ok = mBatch.AddElement(vertex);
    LD_DEBUG_ASSERT(ok);
    return ok;
}

bool RectBatch::AddTexture(const Rect2D& rect, const Rect2D& texRegion, Vec2 texSize, Vec4 color, int texID)
{
    RectVertex vertex[4];

    float u0 = texRegion.x / texSize.x;
    float v0 = texRegion.y / texSize.y;
    float u1 = (texRegion.x + texRegion.w) / texSize.x;
    float v1 = (texRegion.y + texRegion.h) / texSize.y;

    for (int i = 0; i < 4; i++)
    {
        float x = sUnitRectPos[i].x + 0.5f;
        float y = sUnitRectPos[i].y + 0.5f;

        vertex[i].Color = color;
        vertex[i].Position.x = (x * rect.w) + rect.x;
        vertex[i].Position.y = (y * rect.h) + rect.y;
        vertex[i].TexID = (float)texID;
    }

    vertex[0].TexUV = { u1, v0 };
    vertex[1].TexUV = { u1, v1 };
    vertex[2].TexUV = { u0, v1 };
    vertex[3].TexUV = { u0, v0 };

    bool ok = mBatch.AddElement(vertex);
    LD_DEBUG_ASSERT(ok);
    return ok;
}

bool RectBatch::AddGlyph(const Vec2& cursor, const FontGlyph& glyph, float scale, Vec4 color, int texID)
{
    if (mBatch.GetElementCapacity() == mBatch.GetElementCount())
        return false;

    RectVertex vertex[4];
    float u0 = glyph.RectUV.x;
    float v0 = glyph.RectUV.y;
    float u1 = glyph.RectUV.x + glyph.RectUV.w;
    float v1 = glyph.RectUV.y + glyph.RectUV.h;

    // glyph bounding box top left corner, derived from baseline cursor and glyph bearing
    float gx = cursor.x + glyph.BearingX * scale;
    float gy = cursor.y - glyph.BearingY * scale;

    // glyph rendered size
    float gw = glyph.RectXY.w * scale;
    float gh = glyph.RectXY.h * scale;

    vertex[0].Color = color;
    vertex[0].TexID = (float)texID;
    vertex[0].TexUV = { u0, v0 };
    vertex[0].Position = { gx, gy };
    vertex[1].Color = color;
    vertex[1].TexID = (float)texID;
    vertex[1].TexUV = { u0, v1 };
    vertex[1].Position = { gx, gy + gh };
    vertex[2].Color = color;
    vertex[2].TexID = (float)texID;
    vertex[2].TexUV = { u1, v1 };
    vertex[2].Position = { gx + gw, gy + gh };
    vertex[3].Color = color;
    vertex[3].TexID = (float)texID;
    vertex[3].TexUV = { u1, v0 };
    vertex[3].Position = { gx + gw, gy };

    bool ok = mBatch.AddElement(vertex);
    LD_DEBUG_ASSERT(ok);
    return ok;
}

int RectBatch::GetRectCount()
{
    return mBatch.GetElementCount();
}

int RectBatch::GetRectCommitedCount()
{
    return mRectCommited;
}

void RectBatch::Commit()
{
    int rectCount = mBatch.GetElementCount();

    if (rectCount == mRectCommited)
        return;

    int toCommit = rectCount - mRectCommited;
    u32 dataOffset = sizeof(RectVertex) * 4 * mRectCommited;
    u32 dataSize = sizeof(RectVertex) * 4 * toCommit;
    const void* data = mBatch.GetVertices() + 4 * mRectCommited;

    mVertexBuffer.SetData(dataOffset, dataSize, data);
    mRectCommited = rectCount;
}

void RectBatch::GetBuffers(RBuffer& vertexBuffer, RBuffer& indexBuffer)
{
    vertexBuffer = mVertexBuffer;
    indexBuffer = mIndexBuffer;
}

RectBatcher::RectBatcher()
{
}

RectBatcher::~RectBatcher()
{
    LD_DEBUG_ASSERT(!mDevice && mBatches.IsEmpty());
}

void RectBatcher::Startup(RDevice device, int rectCapacity, OnCommit callback)
{
    constexpr int initialBatchCount = 4;

    mDevice = device;
    mBatchCapacity = rectCapacity;
    mBatches.Resize(initialBatchCount);
    mCommitCallback = callback;
    
    for (int i = 0; i < initialBatchCount; i++)
    {
        mBatches[i] = new RectBatch();
        mBatches[i]->Startup(mDevice, mBatchCapacity);
    }
}

void RectBatcher::Cleanup()
{
    for (size_t i = 0; i < mBatches.Size(); i++)
    {
        mBatches[i]->Cleanup();
        delete mBatches[i];
    }

    mBatches.Clear();
    mDevice.ResetHandle();
}

void RectBatcher::Reset()
{
    for (size_t i = 0; i < mBatches.Size(); i++)
    {
        mBatches[i]->Reset();
    }

    mBatchCtr = 0;
}

void RectBatcher::Commit()
{
    RectBatch* batch = mBatches[mBatchCtr];

    int rectCount = batch->GetRectCount();
    int commitedCount = batch->GetRectCommitedCount();
    int toCommit = rectCount - commitedCount;

    if (toCommit == 0)
        return;

    RBuffer vbo, ibo;
    batch->Commit();
    batch->GetBuffers(vbo, ibo);
    mCommitCallback(vbo, ibo, commitedCount * 6, toCommit * 6);
}

void RectBatcher::AddCustom(const RectVertex* vertices)
{
    RectBatch* batch = GetRectBatch(1);

    bool ok = batch->AddCustom(vertices);
    LD_DEBUG_ASSERT(ok);
}

void RectBatcher::AddRectOutline(const Rect2D& rect, Vec4 color, float lineWidth)
{
    RectBatch* batch = GetRectBatch(4);

    bool ok = batch->AddRectOutline(rect, color, lineWidth);
    LD_DEBUG_ASSERT(ok);
}

void RectBatcher::AddRectFilled(const Rect2D& rect, Vec4 color)
{
    RectBatch* batch = GetRectBatch(1);
    
    bool ok = batch->AddRectFilled(rect, color);
    LD_DEBUG_ASSERT(ok);
}

void RectBatcher::AddTexture(const Rect2D& rect, const Rect2D& texRegion, Vec2 texSize, Vec4 color, int texID)
{
    RectBatch* batch = GetRectBatch(1);

    bool ok = batch->AddTexture(rect, texRegion, texSize, color, texID);
    LD_DEBUG_ASSERT(ok);
}

void RectBatcher::AddGlyph(const Vec2& cursor, const FontGlyph& glyph, float scale, Vec4 color, int texID)
{
    RectBatch* batch = GetRectBatch(1);

    bool ok = batch->AddGlyph(cursor, glyph, scale, color, texID);
    LD_DEBUG_ASSERT(ok);
}

RectBatch* RectBatcher::GetRectBatch(int reserve)
{
    RectBatch* batch = mBatches[mBatchCtr];

    if (mBatchCapacity - batch->GetRectCount() < reserve)
    {
        // last commit of this batch, start a new one
        Commit();
        mBatchCtr++;

        if (mBatchCtr == mBatches.Size())
        {
            mBatches.PushBack(new RectBatch());
            mBatches.Back()->Startup(mDevice, mBatchCapacity);
        }

        batch = mBatches[mBatchCtr];
    }

    LD_DEBUG_ASSERT(mBatchCapacity - batch->GetRectCount() >= reserve);
    return batch;
}

RectPipeline::RectPipeline()
{
}

RectPipeline::~RectPipeline()
{
    LD_DEBUG_ASSERT(!mDevice);
}

void RectPipeline::Startup(const RectPipelineInfo& info)
{
    mDevice = info.Device;
    RBackend backend = mDevice.GetBackend();

    RVertexBufferSlot slot{};
    Array<RVertexAttribute, 4> attributes{
        RVertexAttribute{ 0, RDataType::Vec2, false },  // Pos
        RVertexAttribute{ 1, RDataType::Vec2, false },  // TexUV
        RVertexAttribute{ 2, RDataType::Vec4, false },  // Color
        RVertexAttribute{ 3, RDataType::Float, false }, // TexID
    };
    slot.PollRate = RAttributePollRate::PerVertex;
    slot.Attributes = { attributes.Size(), attributes.Data() };

    const char* vsData;
    unsigned int vsSize;
    const char* fsData;
    unsigned int fsSize;

    if (backend == RBackend::Vulkan)
    {
        Embed::GetRectVKVS(&vsSize, &vsData);
        Embed::GetRectVKFS(&fsSize, &fsData);
    }
    else
    {
        Embed::GetRectGLVS(&vsSize, &vsData);
        Embed::GetRectGLFS(&fsSize, &fsData);
    }

    RShaderInfo vertexSI;
    vertexSI.SourceType = RShaderSourceType::SPIRV;
    vertexSI.Type = RShaderType::VertexShader;
    vertexSI.Data = vsData;
    vertexSI.Size = vsSize;
    mDevice.CreateShader(mRectVS, vertexSI);

    RShaderInfo fragmentSI;
    fragmentSI.SourceType = RShaderSourceType::SPIRV;
    fragmentSI.Type = RShaderType::FragmentShader;
    fragmentSI.Data = fsData;
    fragmentSI.Size = fsSize;
    mDevice.CreateShader(mRectFS, fragmentSI);

    RPipelineInfo pipelineI{};
    pipelineI.Name = "RectPipeline";
    pipelineI.VertexLayout.Slots = { 1, &slot };
    pipelineI.VertexShader = mRectVS;
    pipelineI.FragmentShader = mRectFS;
    pipelineI.PrimitiveTopology = RPrimitiveTopology::TriangleList;
    pipelineI.PipelineLayout = info.RectPipelineLayout;
    pipelineI.RenderPass = info.RenderPass;
    pipelineI.DepthStencilState.DepthTestEnabled = false;
    pipelineI.BlendState.BlendEnabled = true;
    pipelineI.BlendState.ColorSrcFactor = RBlendFactor::SrcAlpha;
    pipelineI.BlendState.ColorDstFactor = RBlendFactor::OneMinusSrcAlpha;
    pipelineI.BlendState.ColorBlendMode = RBlendMode::Add;
    pipelineI.BlendState.AlphaSrcFactor = RBlendFactor::One;
    pipelineI.BlendState.AlphaDstFactor = RBlendFactor::Zero;
    pipelineI.BlendState.AlphaBlendMode = RBlendMode::Add;
    mDevice.CreatePipeline(mHandle, pipelineI);
}

void RectPipeline::Cleanup()
{
    mDevice.DeleteShader(mRectFS);
    mDevice.DeleteShader(mRectVS);
    mDevice.DeletePipeline(mHandle);
    mDevice.ResetHandle();
}

RPipelineLayoutData RectPipeline::GetLayoutData() const
{
    RBindingGroupLayoutData group0 = ViewportGroup{}.GetLayoutData();
    RBindingGroupLayoutData group1 = RectGroup{}.GetLayoutData();
    RPipelineLayoutData data{};
    data.GroupLayouts = { group0, group1 };
    
    return data;
}

} // namespace LD

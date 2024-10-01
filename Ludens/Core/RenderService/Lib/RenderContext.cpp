#include "Core/RenderService/Lib/RenderContext.h"

#define RECT_BATCH_CAPACITY 8192

namespace LD
{

namespace Embed
{

void GetDMSans_Regular(unsigned int* size, const char** data);

} // namespace Embed

// clang-format off
static float sQuadVertices[]{
     1.0f,  1.0f,  1.0f, 1.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f,
    -1.0f,  1.0f,  0.0f, 1.0f,
};

static float sCubeVertices[]{
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f,  1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};
// clang-format on

void RenderContext::Startup(RDevice device, int viewportWidth, int viewportHeight)
{
    LD_DEBUG_ASSERT(device);
    Device = device;

    ViewportWidth = viewportWidth;
    ViewportHeight = viewportHeight;
    Device.ResizeViewport(ViewportWidth, ViewportHeight);

    Passes.Startup(device);
    FrameBuffers.Startup(Device, &Passes);
    BindingGroups.Startup(Device);
    Pipelines.Startup(Device, &Passes, &BindingGroups);
    Textures.Startup(Device);

    {
        int vw = ViewportWidth;
        int vh = ViewportHeight;

        FrameBuffers.CreateGBuffer(DefaultGBuffer, vw, vh);
        FrameBuffers.CreateSSAOBuffer(DefaultSSAOBuffer, vw, vh, &Passes.GetSSAOPass());
        FrameBuffers.CreateSSAOBuffer(DefaultSSAOBlurBuffer, vw, vh, &Passes.GetSSAOPass());
        FrameBuffers.CreateColorBuffer(ColorBufferHDR, vw, vh, &Passes.GetColorPassHDR());
        FrameBuffers.CreateColorBuffer(ColorBufferLDR, vw, vh, &Passes.GetColorPassLDR());

        WorldViewportGroup.Startup(Device, BindingGroups.GetViewportBGL());
        WorldViewportGroup.BindGBuffer(DefaultGBuffer);
        WorldViewportGroup.BindSSAOTexture(DefaultSSAOBlurBuffer.GetTexture());

        ScreenViewportGroup.Startup(Device, BindingGroups.GetViewportBGL());
        ScreenViewportGroup.BindColorTextures(ColorBufferHDR, ColorBufferLDR);

        SSAOGroup& ssaoGroup = BindingGroups.GetSSAOGroup();
        ssaoGroup.BindSSAOTexture(DefaultSSAOBuffer.GetTexture());

        RBufferInfo info;
        info.Type = RBufferType::VertexBuffer;
        info.MemoryUsage = RMemoryUsage::Immutable;
        info.Data = sQuadVertices;
        info.Size = sizeof(sQuadVertices);
        Device.CreateBuffer(QuadVBO, info);

        info.Type = RBufferType::VertexBuffer;
        info.MemoryUsage = RMemoryUsage::Immutable;
        info.Data = sCubeVertices;
        info.Size = sizeof(sCubeVertices);
        Device.CreateBuffer(CubeVBO, info);

        unsigned int ttfSize;
        const char* ttfData;
        Embed::GetDMSans_Regular(&ttfSize, &ttfData);

        FontTTFInfo ttfI;
        ttfI.Name = "DMSansRegular";
        ttfI.TTFData = (const void*)ttfData;
        ttfI.TTFSize = (size_t)ttfSize;
        ttfI.PixelSize = 24.0f;
        DefaultFontTTF = MakeRef<FontTTF>(ttfI);

        RFontAtlasInfo atlasI;
        atlasI.Device = Device;
        atlasI.FontData = DefaultFontTTF;
        DefaultFontAtlas.Startup(atlasI);

        // one draw call per commit
        auto onRectBatchCommit = [&](RBuffer vbo, RBuffer ibo, int indexStart, int indexCount)
        {
            Device.SetVertexBuffer(0, vbo);
            Device.SetIndexBuffer(ibo, RIndexType::u16);

            RDrawIndexedInfo drawI;
            drawI.IndexStart = indexStart;
            drawI.IndexCount = indexCount;
            Device.DrawIndexed(drawI);
        };

        DefaultRectBatcher.Startup(Device, RECT_BATCH_CAPACITY, onRectBatchCommit);
        DefaultRectGroup.Startup(Device, BindingGroups.GetRectBGL());

        for (int i = 0; i < 16; i++)
            DefaultRectGroup.BindTexture(Textures.GetWhitePixel(), i);

        DefaultRectGroup.BindTexture(DefaultFontAtlas.GetAtlas(), 1);
    }
}

void RenderContext::Cleanup()
{
    Device.WaitIdle();

    {
        DefaultRectBatcher.Cleanup();
        DefaultRectGroup.Cleanup();
        DefaultFontAtlas.Cleanup();
        DefaultFontTTF = nullptr;
        Device.DeleteBuffer(QuadVBO);
        Device.DeleteBuffer(CubeVBO);
        ScreenViewportGroup.Cleanup();
        WorldViewportGroup.Cleanup();
        DefaultGBuffer.Cleanup();
        DefaultSSAOBuffer.Cleanup();
        DefaultSSAOBlurBuffer.Cleanup();
        ColorBufferHDR.Cleanup();
        ColorBufferLDR.Cleanup();
    }

    Textures.Cleanup();
    Pipelines.Cleanup();
    BindingGroups.Cleanup();
    FrameBuffers.Cleanup();
    Passes.Cleanup();

    Device.ResetHandle();
}

void RenderContext::OnViewportResize(int viewportWidth, int viewportHeight)
{
    ViewportWidth = viewportWidth;
    ViewportHeight = viewportHeight;

    // recreate swapchain
    Device.ResizeViewport(ViewportWidth, ViewportHeight);

    if (DefaultGBuffer)
        DefaultGBuffer.Cleanup();
    FrameBuffers.CreateGBuffer(DefaultGBuffer, ViewportWidth, ViewportHeight);

    if (DefaultSSAOBuffer)
        DefaultSSAOBuffer.Cleanup();
    FrameBuffers.CreateSSAOBuffer(DefaultSSAOBuffer, ViewportWidth, ViewportHeight, &Passes.GetSSAOPass());

    if (DefaultSSAOBlurBuffer)
        DefaultSSAOBlurBuffer.Cleanup();
    FrameBuffers.CreateSSAOBuffer(DefaultSSAOBlurBuffer, ViewportWidth, ViewportHeight, &Passes.GetSSAOPass());

    if (ColorBufferHDR)
        ColorBufferHDR.Cleanup();
    FrameBuffers.CreateColorBuffer(ColorBufferHDR, ViewportWidth, ViewportHeight, &Passes.GetColorPassHDR());

    if (ColorBufferLDR)
        ColorBufferLDR.Cleanup();
    FrameBuffers.CreateColorBuffer(ColorBufferLDR, ViewportWidth, ViewportHeight, &Passes.GetColorPassLDR());

    // make gbuffer results visible from the viewport group
    WorldViewportGroup.BindGBuffer(DefaultGBuffer);

    // make ssao results visible from the viewport group
    SSAOGroup& ssaoGroup = BindingGroups.GetSSAOGroup();
    ssaoGroup.BindSSAOTexture(DefaultSSAOBuffer.GetTexture());
    WorldViewportGroup.BindSSAOTexture(DefaultSSAOBlurBuffer.GetTexture());

    // make HDR and LDR results visible from the viewport group
    ScreenViewportGroup.BindColorTextures(ColorBufferHDR, ColorBufferLDR);
}

} // namespace LD
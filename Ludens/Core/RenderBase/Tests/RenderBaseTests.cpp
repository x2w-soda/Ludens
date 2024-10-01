#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include "Core/Application/Include/Application.h"
#include "Core/Application/Include/Event.h"
#include "Core/DSA/Include/Array.h"
#include "Core/RenderBase/Include/RDevice.h"
#include "Core/RenderBase/Include/RBuffer.h"
#include "Core/RenderBase/Include/RTexture.h"
#include "Core/RenderBase/Include/RShader.h"
#include "Core/RenderBase/Include/RFrameBuffer.h"
#include "Core/RenderBase/Include/RBinding.h"
#include "Core/RenderBase/Include/RPipeline.h"

using namespace LD;

// boilerplate setup and cleanup code, prepare template resources for testing
struct TestResources
{
    void Startup(RBackend backend);
    void Cleanup(bool deleteDevice);

    bool IsEmpty() const
    {
        return !Device && !Texture && !IndexBuffer && !VertexBuffer && !VertexShader && !FragmentShader &&
               !FrameBuffer && !MaterialBGL && !MaterialGroup && !Pipeline;
    }

    RDevice Device;
    RTexture Texture;
    RBuffer IndexBuffer;
    RBuffer VertexBuffer;
    RShader VertexShader;
    RShader FragmentShader;
    RFrameBuffer FrameBuffer;
    RBindingGroupLayout MaterialBGL;
    RBindingGroup MaterialGroup;
    RPipeline Pipeline;
};

void TestResources::Startup(RBackend backend)
{
    RDeviceInfo deviceI{};
    RResult result;

    REQUIRE(IsEmpty());

    deviceI.Backend = backend;
    result = CreateRenderDevice(Device, deviceI);
    CHECK(result);

    RBufferInfo bufferI{};
    bufferI.Type = RBufferType::IndexBuffer;
    bufferI.Data = nullptr;
    bufferI.Size = sizeof(u32) * 6;
    result = Device.CreateBuffer(IndexBuffer, bufferI);
    CHECK(result);

    bufferI.Type = RBufferType::VertexBuffer;
    bufferI.Data = nullptr;
    bufferI.Size = sizeof(float) * 36;
    result = Device.CreateBuffer(VertexBuffer, bufferI);
    CHECK(result);

    RTextureInfo textureI{};
    textureI.Type = RTextureType::Texture2D;
    textureI.Width = 4096;
    textureI.Height = 4096;
    textureI.Format = RTextureFormat::RGBA8;
    textureI.Data = nullptr;
    result = Device.CreateTexture(Texture, textureI);
    CHECK(result);

    RFrameBufferInfo frameBufferI{};

    /* TODO:
    RAttachmentInfo colorAttachments[]{
        { RTextureFormat::RGBA16F },
        { RTextureFormat::RGBA8 },
    };
    frameBufferI.ColorAttachmentInfos = { 2, colorAttachments };
    frameBufferI.Width = 1600;
    frameBufferI.Height = 900;
    result = Device.CreateFrameBuffer(FrameBuffer, frameBufferI);
    CHECK(result);
    */

    Array<RBindingInfo, 4> bindings{
        { RBindingType::UniformBuffer }, // flat colors and params
        { RBindingType::Texture },       // albedo
        { RBindingType::Texture },       // diffuse
        { RBindingType::Texture },       // specular
    };

    RBindingGroupLayoutInfo BGLI{};
    BGLI.Bindings = bindings.GetView();

    result = Device.CreateBindingGroupLayout(MaterialBGL, BGLI);
    CHECK(result);

    RBindingGroupInfo materialBGI;
    materialBGI.Layout = MaterialBGL;
    result = Device.CreateBindingGroup(MaterialGroup, materialBGI);
    CHECK(result);

    static const char sGLSLVertex[] =
        "#version 450 core\nlayout (location=0) in vec2 aPos;    void main() { gl_Position = vec4(aPos, 0.0, 1.0); }";
    static const char sGLSLFragment[] =
        "#version 450 core\nlayout (location=0) out vec4 fColor; void main() { fColor = vec4(1.0); }";

    RShaderInfo shaderI{};
    shaderI.Type = RShaderType::VertexShader;
    shaderI.SourceType = RShaderSourceType::GLSL;
    shaderI.Data = sGLSLVertex;
    shaderI.Size = sizeof(sGLSLVertex);
    result = Device.CreateShader(VertexShader, shaderI);
    CHECK(result);

    RShader fragmentShader;
    shaderI.Type = RShaderType::FragmentShader;
    shaderI.SourceType = RShaderSourceType::GLSL;
    shaderI.Data = sGLSLFragment;
    shaderI.Size = sizeof(sGLSLFragment);
    result = Device.CreateShader(FragmentShader, shaderI);
    CHECK(result);

    RPipelineInfo pipelineI;
    RVertexBufferSlot vertexBufferSlot{};
    RVertexAttribute attr{ 0, RDataType::Vec2, false };
    vertexBufferSlot.Attributes = { 1, &attr };
    pipelineI.VertexShader = VertexShader;
    pipelineI.FragmentShader = FragmentShader;
    pipelineI.VertexLayout.Slots = { 1, &vertexBufferSlot };
    pipelineI.PipelineLayout.GroupLayouts = { 1, &MaterialBGL };
    result = Device.CreatePipeline(Pipeline, pipelineI);
    CHECK(result);
}

void TestResources::Cleanup(bool deleteDevice)
{
    RResult result;

    if (!Device)
        return;

    if (Pipeline)
    {
        result = Device.DeletePipeline(Pipeline);
        CHECK(result);
    }

    if (FragmentShader)
    {
        result = Device.DeleteShader(FragmentShader);
        CHECK(result);
    }

    if (VertexShader)
    {
        result = Device.DeleteShader(VertexShader);
        CHECK(result);
    }

    if (MaterialGroup)
    {
        result = Device.DeleteBindingGroup(MaterialGroup);
        CHECK(result);
    }

    if (MaterialBGL)
    {
        result = Device.DeleteBindingGroupLayout(MaterialBGL);
        CHECK(result);
    }

    if (FrameBuffer)
    {
        result = Device.DeleteFrameBuffer(FrameBuffer);
        CHECK(result);
    }

    if (Texture)
    {
        result = Device.DeleteTexture(Texture);
        CHECK(result);
    }

    if (VertexBuffer)
    {
        result = Device.DeleteBuffer(VertexBuffer);
        CHECK(result);
    }

    if (IndexBuffer)
    {
        result = Device.DeleteBuffer(IndexBuffer);
        CHECK(result);
    }

    if (deleteDevice)
    {
        result = DeleteRenderDevice(Device);
        CHECK(result);
    }
}

// test invalid resource handles
// - creating a new resource from handle that already references another existing resource
// - deleteing a resource but the handle does not reference any resource
static void TestInvalidHandle(RBackend backend)
{
    RResult result;
    TestResources test;
    test.Startup(backend);

    {
        // device already created
        result = CreateRenderDevice(test.Device, {});
        CHECK(result.Type == RResultType::InvalidHandle);

        // buffer already created
        result = test.Device.CreateBuffer(test.VertexBuffer, {});
        CHECK(result.Type == RResultType::InvalidHandle);

        // texture already created
        result = test.Device.CreateTexture(test.Texture, {});
        CHECK(result.Type == RResultType::InvalidHandle);

        // shader already created
        result = test.Device.CreateShader(test.VertexShader, {});
        CHECK(result.Type == RResultType::InvalidHandle);

        // shader already created
        result = test.Device.CreateShader(test.FragmentShader, {});
        CHECK(result.Type == RResultType::InvalidHandle);

        // frame buffer already created
        // result = test.Device.CreateFrameBuffer(test.FrameBuffer, {});
        // CHECK(result.Type == RResultType::InvalidHandle);

        // binding group layout already created
        result = test.Device.CreateBindingGroupLayout(test.MaterialBGL, {});
        CHECK(result.Type == RResultType::InvalidHandle);

        // binding group already created
        result = test.Device.CreateBindingGroup(test.MaterialGroup, {});
        CHECK(result.Type == RResultType::InvalidHandle);

        // pipeline already created
        result = test.Device.CreatePipeline(test.Pipeline, {});
        CHECK(result.Type == RResultType::InvalidHandle);
    }

    test.Cleanup(false);

    {
        // pipeline already deleted
        result = test.Device.DeletePipeline(test.Pipeline);
        CHECK(result.Type == RResultType::InvalidHandle);

        // binding group already deleted
        result = test.Device.DeleteBindingGroup(test.MaterialGroup);
        CHECK(result.Type == RResultType::InvalidHandle);

        // binding group layout already deleted
        result = test.Device.DeleteBindingGroupLayout(test.MaterialBGL);
        CHECK(result.Type == RResultType::InvalidHandle);

        // frame buffer already deleted
        result = test.Device.DeleteFrameBuffer(test.FrameBuffer);
        CHECK(result.Type == RResultType::InvalidHandle);

        // shader already deleted
        result = test.Device.DeleteShader(test.FragmentShader);
        CHECK(result.Type == RResultType::InvalidHandle);

        // shader already deleted
        result = test.Device.DeleteShader(test.VertexShader);
        CHECK(result.Type == RResultType::InvalidHandle);

        // texture already deleted
        result = test.Device.DeleteTexture(test.Texture);
        CHECK(result.Type == RResultType::InvalidHandle);

        // buffer already deleted
        result = test.Device.DeleteBuffer(test.VertexBuffer);
        CHECK(result.Type == RResultType::InvalidHandle);
    }

    result = DeleteRenderDevice(test.Device);
    CHECK(result);

    // device already deleted
    result = DeleteRenderDevice(test.Device);
    CHECK(result.Type == RResultType::InvalidHandle);
}

// test when an index is out of bound
// - for a pipeline with N vertex buffer slots, we can only set vertex buffers to slots [0, N)
// - for a pipeline layout with N binding groups, we can only set binding groups at index [0, N)
// - for a frame buffer with N color attachments, we can only get color attachment at index [0, N)
static void TestInvalidIndex(RBackend backend)
{
    RResult result;
    TestResources test;
    test.Startup(backend);

    result = test.Device.SetPipeline(test.Pipeline);
    CHECK(result);

    // valid vertex buffer slot
    result = test.Device.SetVertexBuffer(0, test.VertexBuffer);
    CHECK(result);

    // invalid vertex buffer slot
    result = test.Device.SetVertexBuffer(1, test.VertexBuffer);
    CHECK(result.Type == RResultType::InvalidIndex);

    // valid binding group slot
    result = test.Device.SetBindingGroup(0, test.MaterialGroup);
    CHECK(result);

    // invalid binding group slot
    result = test.Device.SetBindingGroup(1, test.MaterialGroup);
    CHECK(result.Type == RResultType::InvalidIndex);

    /* TODO:
    RFrameBufferInfo frameBufferI;
    result = test.FrameBuffer.GetInfo(frameBufferI);
    CHECK(result);

    size_t colorAttachmentCount = frameBufferI.ColorAttachmentInfos.Size();
    RTexture attachment;
    CHECK(colorAttachmentCount > 0);

    // valid color attachment index
    result = test.FrameBuffer.GetColorAttachment(0, &attachment);
    CHECK(result);

    // invalid color attachment index
    result = test.FrameBuffer.GetColorAttachment(colorAttachmentCount, &attachment);
    CHECK(result.Type == RResultType::InvalidIndex);
    */

    test.Cleanup(true);
}

// many operations have resource dependencies, check if any resource is missing
// - all draw calls require a bound pipeline
// - setting a vertex buffer requires a bound pipeline
// - setting the index buffer requires a bound pipeline
// - when creating a pipeline, check if RPipelineInfo is fully filled out
static void TestResourceMissing(RBackend backend)
{
    RResult result;
    TestResources test;

    test.Startup(backend);

    // draw calls require a bound pipeline
    {
        result = test.Device.DrawVertex({});
        CHECK(result.Type == RResultType::ResourceMissing);
        CHECK(result.ResourceMissing.MissingType == RResourceType::Pipeline);

        result = test.Device.DrawIndexed({});
        CHECK(result.Type == RResultType::ResourceMissing);
        CHECK(result.ResourceMissing.MissingType == RResourceType::Pipeline);
    }

    // setting a vertex buffer before binding a pipeline
    result = test.Device.SetVertexBuffer(0, test.VertexBuffer);
    CHECK(result.Type == RResultType::ResourceMissing);
    CHECK(result.ResourceMissing.MissingType == RResourceType::Pipeline);

    // setting the index buffer before binding a pipeline
    result = test.Device.SetIndexBuffer(test.IndexBuffer, RIndexType::u32);
    CHECK(result.Type == RResultType::ResourceMissing);
    CHECK(result.ResourceMissing.MissingType == RResourceType::Pipeline);

    // creating a pipeline requires some resources
    {
        RPipeline pipeline;
        RPipelineInfo pipelineI;
        pipelineI.VertexShader = test.VertexShader;

        // missing fragment shader
        result = test.Device.CreatePipeline(pipeline, pipelineI);
        CHECK(result.Type == RResultType::ResourceMissing);
        CHECK(result.ResourceMissing.MissingType == RResourceType::Shader);

        pipelineI.VertexShader.ResetHandle();
        pipelineI.FragmentShader = test.FragmentShader;

        // missing vertex shader
        result = test.Device.CreatePipeline(pipeline, pipelineI);
        CHECK(result.Type == RResultType::ResourceMissing);
        CHECK(result.ResourceMissing.MissingType == RResourceType::Shader);
    }

    test.Cleanup(true);
}

// texture byte size of base mipmap level can be inferred from Width, Height, Depth, and Format
// - when uploading full texture data, validate the input byte size
static void TestTextureSizeMismatch(RBackend backend)
{
    RResult result;
    TestResources test;

    test.Startup(backend);
    {
        static f32 pixels[32 * 32];

        RTexture texture;
        RTextureInfo info{};
        info.Format = RTextureFormat::RGBA16F;
        info.Type = RTextureType::Texture2D;
        info.Width = 32;
        info.Height = 32;

        // bad pixel data
        info.Data = pixels;
        info.Size = sizeof(pixels);

        size_t correctByteSize = 32 * 32 * 4 * 2;
        result = test.Device.CreateTexture(texture, info);
        CHECK(result.Type == RResultType::TextureSizeMismatch);
        CHECK(result.TextureSizeMismatch.Expect == correctByteSize);
        CHECK(result.TextureSizeMismatch.Actual == sizeof(pixels));
    }
    test.Cleanup(true);
}

// most buffer operations require a specific buffer type
// - set vertex buffer
// - set index buffer
static void TestBufferTypeMismatch(RBackend backend)
{
    RResult result;
    TestResources test;
    test.Startup(backend);

    result = test.Device.SetPipeline(test.Pipeline);
    CHECK(result);

    result = test.Device.SetVertexBuffer(0, test.IndexBuffer);
    CHECK(result.Type == RResultType::BufferTypeMismatch);
    CHECK(result.BufferTypeMismatch.Expect == RBufferType::VertexBuffer);
    CHECK(result.BufferTypeMismatch.Actual == RBufferType::IndexBuffer);

    result = test.Device.SetIndexBuffer(test.VertexBuffer, RIndexType::u32);
    CHECK(result.Type == RResultType::BufferTypeMismatch);
    CHECK(result.BufferTypeMismatch.Expect == RBufferType::IndexBuffer);
    CHECK(result.BufferTypeMismatch.Actual == RBufferType::VertexBuffer);

    test.Cleanup(true);
}

// most shader operations require a specific shader type
// - RPipelineInfo requires at least a vertex shader and a fragment shader
static void TestShaderTypeMismatch(RBackend backend)
{
    RResult result;
    TestResources test;
    test.Startup(backend);

    // creating a pipeline but shader types are messed up
    {
        RPipeline pipeline;
        RPipelineInfo pipelineI;

        pipelineI.VertexShader = test.FragmentShader;
        pipelineI.FragmentShader = test.FragmentShader;

        result = test.Device.CreatePipeline(pipeline, pipelineI);
        CHECK(result.Type == RResultType::ShaderTypeMismatch);
        CHECK(result.ShaderTypeMismatch.Expect == RShaderType::VertexShader);
        CHECK(result.ShaderTypeMismatch.Actual == RShaderType::FragmentShader);

        pipelineI.VertexShader = test.VertexShader;
        pipelineI.FragmentShader = test.VertexShader;

        // missing vertex shader
        result = test.Device.CreatePipeline(pipeline, pipelineI);
        CHECK(result.Type == RResultType::ShaderTypeMismatch);
        CHECK(result.ShaderTypeMismatch.Expect == RShaderType::FragmentShader);
        CHECK(result.ShaderTypeMismatch.Actual == RShaderType::VertexShader);
    }

    test.Cleanup(true);
}

// test when we fail to begin a render pass
// - the number of clear values don't match the number of attachments
// - missing a clear value for an attachment with load op Clear
static void TestPassBeginError(RBackend backend)
{
    // TODO:
}

class RResultTestLayer : public ApplicationLayer
{
public:
    void OnAttach(const Application& app) override;
    void OnDeltaUpdate(DeltaTime dt) override;

private:
};

void RResultTestLayer::OnAttach(const Application& app)
{
    RBackend backend = app.GetRendererBackend();

    TestInvalidHandle(backend);
    TestInvalidIndex(backend);
    TestResourceMissing(backend);
    TestTextureSizeMismatch(backend);
    TestBufferTypeMismatch(backend);
    TestShaderTypeMismatch(backend);
}

void RResultTestLayer::OnDeltaUpdate(DeltaTime dt)
{
    // all tests are completed during attach, quit application at first frame
    ApplicationQuitEvent event;
    EventDispatch(event, &Application::EventHandler);
}

TEST_CASE("RResult Error Handling")
{
    ApplicationConfig config{};
    config.Window.Name = "RenderBaseTests";
    config.Window.Width = 1600;
    config.Window.Height = 900;
    config.Window.IsVisible = false;
    config.Layer = MakeRef<RResultTestLayer>();
    config.RendererBackend = RBackend::OpenGL;

    Application& app = Application::GetSingleton();
    app.Run(config);
}
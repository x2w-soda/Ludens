#pragma once

#include "Core/Header/Include/Singleton.h"
#include "Core/Math/Include/Mat4.h"
#include "Core/OS/Include/UID.h"
#include "Core/OS/Include/Memory.h"
#include "Core/Media/Include/Font.h"
#include "Core/RenderBase/Include/RTypes.h"

namespace LD
{

class UIContext;
class Model;

/// renderer resource id
using RRID = UID;

enum class RenderPipeline
{
    BRDF = 0,
    BlinnPhong = 1,
};

enum class LDRResult
{
    /// apply Reinhard tone mapping on HDR texture
    ToneMappedReinhard = 0,

    /// output view space normals as LDR result
    Normals = 2,

    /// output albedo as LDR result
    Albedo = 3,

    /// output metallic as LDR result
    Metallic = 4,

    /// output roughness as LDR result
    Roughness = 5,
};

class RenderService : public Singleton<RenderService>
{
    friend class Singleton<RenderService>;

public:
    void Startup(RBackend backend);
    void Cleanup();

    void GetDefaultFont(Ref<FontTTF>& ttf, Ref<FontGlyphTable>& table);

    void SetDefaultRenderPipeline(RenderPipeline pipeline);
    void SetLDRResult(LDRResult result);

    void BeginFrame();
    void EndFrame();

    void BeginWorldViewport(const Vec3& viewpos, const Mat4& view, const Mat4& projection, RRID cubemap = 0);
    void EndWorldViewport();

    void BeginScreenViewport();
    void EndScreenViewport();

    void CreateCubemap(RRID& id, int resolution, const void* data);
    void DeleteCubemap(RRID id);

    void CreateMesh(RRID& id, Ref<Model> model);
    void DeleteMesh(RRID id);

    void CreateDirectionalLight(RRID& id, const Vec3& direction, const Vec3& color);
    void DeleteDirectionalLight(RRID id);

    void DrawMesh(RRID mesh, const Mat4& transform);

    void DrawScreenUI(UIContext* ui);

private:
    void OnViewportResize(int width, int height);
    void WorldRenderPasses();
    void ScreenRenderPasses();
    void SwapChainRenderPasses();

    struct RenderContext* mCtx;
};

} // namespace LD
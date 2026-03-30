#pragma once

#include <Ludens/AudioSystem/AudioSystem.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderSystem/RenderSystem.h>
#include <Ludens/System/FileSystem.h>
#include <LudensBuilder/ProjectBuilder/ProjectScan.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorUI/EditorUI.h>

namespace LD {

struct EditorApplicationInfo
{
    bool vsync = true;
    const FS::Path* projectSchemaPath = nullptr;
};

class EditorApplication
{
public:
    EditorApplication() = delete;
    EditorApplication(const EditorApplicationInfo& info);
    EditorApplication(const EditorApplication&) = delete;
    EditorApplication(EditorApplication&&) = delete;
    ~EditorApplication();

    EditorApplication& operator=(const EditorApplication&) = delete;
    EditorApplication& operator=(EditorApplication&&) = delete;

    void run();

private:
    void begin_project_scans(const Vector<FS::Path>& projectSchemas);
    void wait_project_scans(Vector<ProjectScanResult>& projectScanResults);

private:
    RDevice mRDevice;
    RenderSystem mRenderSystem;
    AudioSystem mAudioSystem;
    EditorContext mEditorCtx;
    EditorUI mEditorUI;
    Font mFont;
    Font mMSFont;
    FontAtlas mFontAtlas;
    FontAtlas mMSFontAtlas;
    RImage mFontAtlasImage;
    RImage mMSFontAtlasImage;
    ImageCube mEnvCubemap = {};
    Vector<ProjectScanAsync> mProjectScans;
};

} // namespace LD
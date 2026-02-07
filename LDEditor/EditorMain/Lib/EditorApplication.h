#pragma once

#include <Ludens/AudioSystem/AudioSystem.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderSystem/RenderSystem.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorUI/EditorUI.h>

namespace LD {

class EditorApplication
{
public:
    EditorApplication();
    EditorApplication(const EditorApplication&) = delete;
    EditorApplication(EditorApplication&&) = delete;
    ~EditorApplication();

    EditorApplication& operator=(const EditorApplication&) = delete;
    EditorApplication& operator=(EditorApplication&&) = delete;

    void run();

private:
    RDevice mRDevice;
    RenderSystem mRenderSystem;
    AudioSystem mAudioSystem;
    EditorContext mEditorCtx;
    EditorUI mEditorUI;
    Font mFont;
    FontAtlas mFontAtlas;
    RImage mFontAtlasImage;
    ImageCube mEnvCubemap;
};

} // namespace LD
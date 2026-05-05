#pragma once

#include <Ludens/AudioSystem/AudioSystem.h>
#include <Ludens/Lua/LuaState.h>
#include <Ludens/RenderSystem/RenderSystem.h>
#include <Ludens/Scene/SceneCommand.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

class LuaSceneDriver
{
public:
    LuaSceneDriver() = delete;
    LuaSceneDriver(RenderSystem renderSystem, AudioSystem audioSystem, FontAtlas atlas);
    LuaSceneDriver(const LuaSceneDriver&) = delete;
    ~LuaSceneDriver();

    bool run(const FS::Path& luaFile);

    LuaSceneDriver& operator=(const LuaSceneDriver&) = delete;

private:
    LuaState mL;
    RenderSystem mRenderSystem;
    AudioSystem mAudioSystem;
    SceneCommandQueue mCommandQueue;
    FontAtlas mFontAtlas;
};

} // namespace LD
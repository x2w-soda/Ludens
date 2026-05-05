#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Scene.h>

#include "LuaSceneCommand.h"
#include "LuaSceneDriver.h"

namespace LD {

Log sLog("LuaSceneDriver");

LuaSceneDriver::LuaSceneDriver(RenderSystem renderSystem, AudioSystem audioSystem, FontAtlas atlas)
    : mRenderSystem(renderSystem), mAudioSystem(audioSystem), mFontAtlas(atlas)
{
    LuaStateInfo stateI{};
    stateI.openLibs = true;
    mL = LuaState::create(stateI);

    mCommandQueue = SceneCommandQueue::create();
}

LuaSceneDriver::~LuaSceneDriver()
{
    SceneCommandQueue::destroy(mCommandQueue);
    LuaState::destroy(mL);
}

bool LuaSceneDriver::run(const FS::Path& luaFile)
{
    LD_PROFILE_SCOPE;

    mL.clear();

    sLog.info("begin {}", FS::absolute(luaFile).string());

    if (!mL.do_file(luaFile.string().c_str()))
    {
        sLog.error("error {}", mL.to_string(-1));
        return false;
    }

    std::string err;
    Vector<LuaSceneCommand*> cmds;
    if (!parse_lua_scene_commands(mL, cmds, err))
        return false;

    UIFontRegistry fontReg = UIFontRegistry::create();
    SUIDRegistry suidReg = SUIDRegistry::create();
    SceneInfo sceneI{};
    sceneI.audioSystem = mAudioSystem;
    sceneI.renderSystem = mRenderSystem;
    sceneI.suidRegistry = suidReg;
    sceneI.uiFont = fontReg.add_font(mFontAtlas, mRenderSystem.get_font_atlas_image());
    sceneI.uiTheme = UITheme::get_default_theme();
    Scene scene = Scene::create(sceneI);
    if (!scene)
        return false;

    bool success = true;

    // execute each command
    for (auto* cmd : cmds)
    {
        (void)execute_lua_scene_command(cmd, mCommandQueue, scene, err);

        if (!err.empty())
        {
            sLog.error("command failed with: {}", err);
            success = false;
            break;
        }
    }

    free_lua_scene_commands(cmds);

    Scene::destroy();
    SUIDRegistry::destroy(suidReg);
    UIFontRegistry::destroy(fontReg);

    if (success)
        sLog.info("success");

    return success;
}

} // namespace LD
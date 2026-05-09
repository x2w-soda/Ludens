#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Scene.h>

#include "LuaSceneCommand.h"
#include "LuaSceneDriver.h"
#include "LuaWindowCommand.h"

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

/// Each step in a SceneTest is either a SceneCommand or WindowCommand.
bool LuaSceneDriver::step(String& err)
{
    int oldSize = mL.size();

    if (mL.get_field_type(-1, "scene_command", LUA_TYPE_STRING))
    {
        String cmdName = mL.to_string(-1);
        SceneCommandType cmdType = SceneCommand::get_type(cmdName.c_str());
        if (cmdType == SCENE_COMMAND_TYPE_ENUM_COUNT)
        {
            err = std::format("unknown scene_command type: {}", cmdName.c_str()).c_str();
            return false;
        }
        mL.pop(1);

        LuaSceneCommand* cmd = LuaSceneCommand::create(mL, err);
        if (!cmd)
            return false;

        (void)LuaSceneCommand::execute(cmd, mCommandQueue, mScene, err);
        LuaSceneCommand::destroy(cmd);

        if (!err.empty())
        {
            sLog.error("LuaSceneCommand {} failed with:\n{}", SceneCommand::get_name(cmdType), err.c_str());
            LD_DEBUG_BREAK;
            return false;
        }
    }
    else if (mL.get_field_type(-1, "window_command", LUA_TYPE_STRING))
    {
        String cmdName = mL.to_string(-1);
        LuaWindowCommandType cmdType = LuaWindowCommand::get_type(cmdName.c_str());
        if (cmdType == SCENE_COMMAND_TYPE_ENUM_COUNT)
        {
            err = std::format("unknown window_command type: {}", cmdName.c_str()).c_str();
            return false;
        }
        mL.pop(1);

        LuaWindowCommand* cmd = LuaWindowCommand::create(mL, err);
        if (!cmd)
            return false;

        (void)LuaWindowCommand::execute(cmd, err);
        LuaWindowCommand::destroy(cmd);

        if (!err.empty())
        {
            sLog.error("LuaWindowCommand {} failed with:\n{}", LuaWindowCommand::get_name(cmdType), err.c_str());
            LD_DEBUG_BREAK;
            return false;
        }
    }
    else
    {
        LD_DEBUG_BREAK;
        err = "unknown step";
        return false;
    }

    mL.resize(oldSize);

    const Vec2 testDriverSceneExtent(1600, 900);
    SceneUpdateTick tick{};
    tick.extent = testDriverSceneExtent;
    mScene.update(tick);

    return true;
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

    if (!mL.get_field_type(-1, "steps", LUA_TYPE_TABLE))
    {
        sLog.error("missing steps table");
        return false;
    }

    UIFontRegistry fontReg = UIFontRegistry::create();
    SUIDRegistry suidReg = SUIDRegistry::create();
    SceneInfo sceneI{};
    sceneI.audioSystem = mAudioSystem;
    sceneI.renderSystem = mRenderSystem;
    sceneI.suidRegistry = suidReg;
    sceneI.uiFont = fontReg.add_font(mFontAtlas, mRenderSystem.get_font_atlas_image());
    sceneI.uiTheme = UITheme::get_default_theme();
    mScene = Scene::create(sceneI);
    if (!mScene)
        return false;

    String err;
    bool success = true;
    int stepI = 1;
    int oldSize = mL.size();
    mL.push_number(stepI);
    mL.get_table(-2);
    while (mL.get_type(-1) == LUA_TYPE_TABLE)
    {
        if (!step(err))
        {
            sLog.error("step {} failed with: {}", stepI, err.c_str());
            success = false;
            break;
        }

        mL.resize(oldSize);
        mL.push_number(++stepI);
        mL.get_table(-2);
    }
    mL.resize(oldSize);

    mScene.cleanup();

    Scene::destroy();
    SUIDRegistry::destroy(suidReg);
    UIFontRegistry::destroy(fontReg);

    if (success)
        sLog.info("success");

    return success;
}

} // namespace LD
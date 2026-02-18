set(MODULE_NAME LDScene)
set(MODULE_TEST_NAME LDSceneTest)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/Scene/Scene.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Scene/SceneSchema.h
)

set(MODULE_LIB
    Lib/SceneObj.h
    Lib/Scene.cpp
    Lib/SceneSchemaKeys.h
    Lib/SceneSchema.cpp
    Lib/ScreenUI.h
    Lib/ScreenUI.cpp
    Lib/UIDriver.h
    Lib/UIDriver.cpp
    Lib/AudioSystemCache.h
    Lib/AudioSystemCache.cpp
    Lib/RenderSystemCache.h
    Lib/RenderSystemCache.cpp
    Lib/LuaScript.h
    Lib/LuaScript.cpp
    Lib/LuaScriptFFI.h
    Lib/LuaScriptFFI.cpp
    Lib/Component/MeshComponent.h
    Lib/Component/MeshComponent.cpp
    Lib/Component/CameraComponent.h
    Lib/Component/CameraComponent.cpp
    Lib/Component/AudioSourceComponent.h
    Lib/Component/AudioSourceComponent.cpp
    Lib/Component/Sprite2DComponent.h
    Lib/Component/Sprite2DComponent.cpp
    Lib/Component/ScreenUIComponent.h
    Lib/Component/ScreenUIComponent.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDProfiler
    LDSystem
    LDLua
    LDMedia
    LDCamera
    LDDataRegistry
    LDWindowRegistry
)

set(MODULE_TEST
    Test/SceneTest.cpp
    Test/SceneSchemaTest.cpp
    Test/SceneLuaScriptFFITest.cpp
)

add_ludens_core_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
)
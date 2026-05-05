set(MODULE_NAME LDScene)
set(MODULE_TEST_NAME LDSceneTest)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/Scene/Scene.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Scene/SceneDef.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Scene/SceneSchema.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Scene/ComponentView.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Scene/ComponentViews.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Scene/Component/MeshView.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Scene/Component/CameraView.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Scene/Component/Camera2DView.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Scene/Component/AudioSourceView.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Scene/Component/Transform2DView.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Scene/Component/Sprite2DView.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Scene/Component/ScreenUIView.h
)

set(MODULE_LIB
    Lib/SceneObj.h
    Lib/Scene.cpp
    Lib/SceneSchemaKeys.h
    Lib/SceneSchema.cpp
    Lib/SceneCommand.cpp
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
    Lib/Component/Camera2DComponent.h
    Lib/Component/Camera2DComponent.cpp
    Lib/Component/AudioSourceComponent.h
    Lib/Component/AudioSourceComponent.cpp
    Lib/Component/Transform2DComponent.h
    Lib/Component/Transform2DComponent.cpp
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
    Test/LuaSceneCommand.h
    Test/LuaSceneCommand.cpp
    Test/LuaSceneDriver.h
    Test/LuaSceneDriver.cpp
    Test/SceneSchemaTest.cpp
    Test/SceneLuaScriptFFITest.cpp
)

add_ludens_core_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
    LDLudensLFS
    LDTestUtil
)
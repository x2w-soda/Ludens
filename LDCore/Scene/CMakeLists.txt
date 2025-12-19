set(MODULE_NAME LDScene)
set(MODULE_TEST_NAME LDSceneTest)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/Scene/Scene.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Scene/SceneSchema.h
)

set(MODULE_LIB
    Lib/Scene.cpp
    Lib/SceneSchema.cpp
    Lib/AudioServerCache.h
    Lib/AudioServerCache.cpp
    Lib/RenderServerCache.h
    Lib/RenderServerCache.cpp
    Lib/ScreenRenderer.h
    Lib/ScreenRenderer.cpp
    Lib/LuaScript.h
    Lib/LuaScript.cpp
    Lib/LuaScriptFFI.h
    Lib/LuaScriptFFI.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDProfiler
    LDSystem
    LDLua
    LDMedia
    LDCamera
    LDDataRegistry
    LDApplication
)

set(MODULE_TEST
    Test/SceneTest.cpp
    Test/SceneSchemaTest.cpp
    Test/SceneLuaScriptFFITest.cpp
)

add_executable(${MODULE_TEST_NAME}
    ${MODULE_TEST}
)

target_include_directories(${MODULE_TEST_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_SOURCE_DIR}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
)
set(MODULE_NAME LDUI)
set(MODULE_TEST_NAME LDUITest)
set(MODULE_SANDBOX_NAME LDUISandbox)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/UI/UIContext.h
    ${LUDENS_INCLUDE_DIR}/Ludens/UI/UILayout.h
    ${LUDENS_INCLUDE_DIR}/Ludens/UI/UIWindow.h
    ${LUDENS_INCLUDE_DIR}/Ludens/UI/UIWidget.h
    ${LUDENS_INCLUDE_DIR}/Ludens/UI/UITheme.h
    ${LUDENS_INCLUDE_DIR}/Ludens/UI/UIWindowManager.h
    ${LUDENS_INCLUDE_DIR}/Ludens/UI/UIAnimation.h
    ${LUDENS_INCLUDE_DIR}/Ludens/UI/UIImmediate.h
)

set(MODULE_LIB
    Lib/UIObj.h
    Lib/UIContext.cpp
    Lib/UILayout.cpp
    Lib/UIWindow.cpp
    Lib/UIWidget.cpp
    Lib/UITheme.cpp
    Lib/UIWindowManager.cpp
    Lib/UIAnimation.cpp
    Lib/UIImmediate.cpp
    Lib/WindowManager/AreaNode.h
    Lib/WindowManager/AreaNode.cpp
    Lib/WindowManager/AreaTab.h
    Lib/WindowManager/AreaTab.cpp
    Lib/WindowManager/UIWindowManagerObj.h
)

set(MODULE_TEST
    Test/UITest.cpp
    Test/UITest.h
    Test/UILayoutTest.cpp
    Test/UIInputTest.cpp
    Test/UITextWidgetTest.cpp
)

set(MODULE_SANDBOX
    Sandbox/UISandboxMain.cpp
    Sandbox/UISandbox.h
    Sandbox/UISandbox.cpp
)

add_library(${MODULE_NAME}
    ${MODULE_INCLUDE}
    ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDSystem
    LDMedia
    LDRenderComponent
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
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

add_executable(${MODULE_SANDBOX_NAME}
    ${MODULE_SANDBOX}
)

target_include_directories(${MODULE_SANDBOX_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_SOURCE_DIR}
)

target_link_libraries(${MODULE_SANDBOX_NAME} PRIVATE
    ${MODULE_NAME}
    LDApplication
    LDRenderServer
    LDLudensLFS
    LDCamera
    LDEditorContext # for the EditorIconAtlas
)
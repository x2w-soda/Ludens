set(MODULE_NAME LDUI)
set(MODULE_TEST_NAME LDUITest)
set(MODULE_SANDBOX_NAME LDUISandbox)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/UI/UIContext.h
    ${LUDENS_INCLUDE_DIR}/Ludens/UI/UILayer.h
    ${LUDENS_INCLUDE_DIR}/Ludens/UI/UIWorkspace.h
    ${LUDENS_INCLUDE_DIR}/Ludens/UI/UIWindow.h
    ${LUDENS_INCLUDE_DIR}/Ludens/UI/UIWidget.h
    ${LUDENS_INCLUDE_DIR}/Ludens/UI/UILayout.h
    ${LUDENS_INCLUDE_DIR}/Ludens/UI/UITheme.h
    ${LUDENS_INCLUDE_DIR}/Ludens/UI/UIAnimation.h
    ${LUDENS_INCLUDE_DIR}/Ludens/UI/UIImmediate.h
)

set(MODULE_LIB
    Lib/UIObj.h
    Lib/UIContext.cpp
    Lib/UILayer.cpp
    Lib/UIWorkspace.cpp
    Lib/UIWindow.cpp
    Lib/UIWidget.cpp
    Lib/UILayout.cpp
    Lib/UITheme.cpp
    Lib/UIAnimation.cpp
    Lib/UIImmediate.cpp
)

set(MODULE_TEST
    Test/UITest.cpp
    Test/UITest.h
    Test/UIContextTest.cpp
    Test/UILayoutTest.cpp
    Test/UIInputTest.cpp
    Test/UITextWidgetTest.cpp
)

set(MODULE_SANDBOX
    Sandbox/UISandboxMain.cpp
    Sandbox/UISandbox.h
    Sandbox/UISandbox.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDSystem
    LDText
    LDMedia
    LDWindowRegistry
    LDRenderComponent
)

add_ludens_core_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
    LDLudensLFS
)

add_executable(${MODULE_SANDBOX_NAME}
    ${MODULE_SANDBOX}
)

set_target_properties(${MODULE_SANDBOX_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_include_directories(${MODULE_SANDBOX_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_SOURCE_DIR}
)

target_link_libraries(${MODULE_SANDBOX_NAME} PRIVATE
    ${MODULE_NAME}
    LDWindowRegistry
    LDRenderServer
    LDLudensLFS
    LDCamera
    LDEditorContext # for the EditorIconAtlas
)

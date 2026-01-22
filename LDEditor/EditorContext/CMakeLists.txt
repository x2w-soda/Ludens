set(MODULE_NAME LDEditorContext)
set(MODULE_TEST_NAME LDEditorContextTest)

set(MODULE_LIB
	Lib/EditorAction.cpp
	Lib/EditorContext.cpp
	Lib/EditorContextCommand.h
	Lib/EditorContextCommand.cpp
	Lib/EditorSettings.cpp
	Lib/EditorIconAtlas.cpp
	Lib/EditStack.cpp
)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditorContextEvent.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditorContext.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditorAction.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditorSettings.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditorWindow.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditorIconAtlas.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditStack.h
)

add_ludens_editor_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

set(MODULE_TEST
    Test/EditorContextTest.h
    Test/EditorContextTest.cpp
    Test/EditorActionTest.cpp
    Test/EditStackTest.cpp
)

add_ludens_editor_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
)
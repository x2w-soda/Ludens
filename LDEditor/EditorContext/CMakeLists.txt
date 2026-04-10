set(MODULE_NAME LDEditorContext)
set(MODULE_TEST_NAME LDEditorContextTest)

set(MODULE_LIB
	Lib/EditorEventQueue.cpp
	Lib/EditorContextObj.h
	Lib/EditorContext.cpp
	Lib/EditorSettings.cpp
	Lib/EditorIconAtlas.cpp
	Lib/EditCommand.cpp
	Lib/EditStack.cpp
)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditorEvent.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditorEventQueue.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditorContextDef.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditorContext.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditorSettings.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditorWindow.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditorIconAtlas.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditCommand.h
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
    Test/EditStackTest.cpp
)

add_ludens_editor_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
)
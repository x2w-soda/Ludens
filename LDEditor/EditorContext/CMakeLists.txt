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

add_library(${MODULE_NAME}
	${MODULE_LIB}
	${MODULE_INCLUDE}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_EDITOR_MODULE_FOLDER})

target_link_libraries(${MODULE_NAME} PRIVATE
	LDCoreLibs
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
)

set(MODULE_TEST
    Test/EditorContextTest.h
    Test/EditorContextTest.cpp
    Test/EditorActionTest.cpp
    Test/EditStackTest.cpp
)

add_executable(${MODULE_TEST_NAME}
    ${MODULE_TEST}
)

set_target_properties(${MODULE_TEST_NAME} PROPERTIES FOLDER ${LD_EDITOR_MODULE_FOLDER})

target_include_directories(${MODULE_TEST_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_SOURCE_DIR}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
)
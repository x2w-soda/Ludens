set(MODULE_NAME LDEditorUI)

set(MODULE_LIB
	Lib/EditorUI.cpp
	Lib/EditorTopBar.cpp
	Lib/EditorWorkspace.cpp
)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorUI/EditorUI.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorUI/EditorTopBar.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorUI/EditorWorkspace.h
)

add_library(${MODULE_NAME}
	${MODULE_LIB}
	${MODULE_INCLUDE}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_EDITOR_MODULE_FOLDER})

target_link_libraries(${MODULE_NAME} PRIVATE
	LDCoreLibs
	LDLudensLFS # TODO: embed required media instead of fetching from submodule
    LDInspectorWindow
    LDVersionWindow
    LDViewportWindow
    LDSelectionWindow
    LDTabControlWindow
    LDOutlinerWindow
    LDConsoleWindow
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
)

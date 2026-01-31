set(MODULE_NAME LDEditorUI)

set(MODULE_LIB
	Lib/EditorUI.cpp
	Lib/EditorUITopBar.cpp
	Lib/EditorUIDialog.cpp
	Lib/EditorUIMain.cpp
	Lib/EditorWorkspace.cpp
	Lib/EditorDialog.cpp
)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorUI/EditorUI.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorUI/EditorUITopBar.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorUI/EditorUIDialog.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorUI/EditorUIMain.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorUI/EditorWorkspace.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorUI/EditorDialog.h
)

add_ludens_editor_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDCoreLibs
	LDLudensLFS # TODO: embed required media instead of fetching from submodule
    LDInspectorWindow
    LDVersionWindow
    LDViewportWindow
    LDSelectionWindow
    LDCreateComponentWindow
    LDProjectSettingsWindow
    LDTabControlWindow
    LDOutlinerWindow
    LDConsoleWindow
)

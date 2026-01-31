set(MODULE_NAME LDProjectSettingsWindow)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensEditor/ProjectSettingsWindow/ProjectSettingsWindow.h
)

set(MODULE_LIB
	Lib/ProjectSettingsWindow.cpp
)

add_ludens_editor_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PRIVATE
    LDEditorContext
    LDEditorWidget
)

set(MODULE_NAME LDFileSelectWindow)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensEditor/FileSelectWindow/FileSelectWindow.h
)

set(MODULE_LIB
	Lib/FileSelectWindow.cpp
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

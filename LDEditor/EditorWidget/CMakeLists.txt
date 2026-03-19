set(MODULE_NAME LDEditorWidget)

set(MODULE_LIB
	Lib/EUI.h
	Lib/EUI.cpp
	Lib/EUIVectorEdit.cpp
	Lib/EUITransformEdit.cpp
	Lib/EUIAssetSlot.cpp
	Lib/EUIListMenu.cpp
)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/EditorWidget.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/EUIVectorEdit.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/EUITransformEdit.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/EUIAssetSlot.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/EUIListMenu.h
)

add_ludens_editor_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDEditorContext
)
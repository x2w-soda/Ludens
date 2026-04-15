set(MODULE_NAME LDEditorWidget)

set(MODULE_LIB
	Lib/EUI.h
	Lib/EUI.cpp
	Lib/EUIRow.cpp
	Lib/EUIIcon.cpp
	Lib/EUIText.cpp
	Lib/EUIScroll.cpp
	Lib/EUIPrimitiveEdit.cpp
	Lib/EUITransformEdit.cpp
	Lib/EUIAssetSlot.cpp
	Lib/EUIListMenu.cpp
	Lib/EUIComponent.cpp
	Lib/EUIDocument.cpp
	Lib/EUISegmentControl.cpp
)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/EditorWidget.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/EUIRow.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/EUIIcon.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/EUIText.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/EUIScroll.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/EUIPrimitiveEdit.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/EUITransformEdit.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/EUIAssetSlot.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/EUIListMenu.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/EUIComponent.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/EUIDocument.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/EUISegmentControl.h
)

add_ludens_editor_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDEditorContext
)
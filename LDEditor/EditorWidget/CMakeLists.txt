set(MODULE_NAME LDEditorWidget)

set(MODULE_LIB
	Lib/UITransformEditWidget.cpp
	Lib/UIAssetSlotWidget.cpp
	Lib/UIListMenuWidget.cpp
	Lib/UIDraw.cpp
)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/UITransformEditWidget.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/UIAssetSlotWidget.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/UIListMenuWidget.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/UIDraw.h
)

add_ludens_editor_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDEditorContext
)
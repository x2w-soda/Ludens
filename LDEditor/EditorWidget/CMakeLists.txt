set(MODULE_NAME LDEditorWidget)

set(MODULE_LIB
	Lib/UITransformEditWidget.cpp
	Lib/UIAssetSlotWidget.cpp
	Lib/UIDropdownWindow.cpp
	Lib/UIVersionWindow.cpp
	Lib/UISelectWindow.cpp
)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/UITransformEditWidget.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/UIAssetSlotWidget.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/UIDropdownWindow.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/UIVersionWindow.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/UISelectWindow.h
)

add_library(${MODULE_NAME}
	${MODULE_LIB}
	${MODULE_INCLUDE}
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDCoreLibs
	LDEditorContext
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
)
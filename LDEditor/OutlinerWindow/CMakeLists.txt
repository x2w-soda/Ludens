set(MODULE_NAME LDOutlinerWindow)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensEditor/OutlinerWindow/OutlinerWindow.h
)

set(MODULE_LIB
	Lib/OutlinerWindow.cpp
	Lib/ComponentMenu.h
	Lib/ComponentMenu.cpp
)

add_ludens_editor_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDEditorContext
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
)
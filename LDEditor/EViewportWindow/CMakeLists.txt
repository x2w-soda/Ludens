set(MODULE_NAME LDEViewportWindow)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensEditor/EViewportWindow/EViewportWindow.h
)

set(MODULE_LIB
	Lib/EViewportWindow.cpp
	Lib/ViewportToolbar.h
	Lib/ViewportToolbar.cpp
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
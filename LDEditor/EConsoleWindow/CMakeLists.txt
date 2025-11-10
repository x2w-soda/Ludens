set(MODULE_NAME LDEConsoleWindow)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensEditor/EConsoleWindow/EConsoleWindow.h
)

set(MODULE_LIB
	Lib/EConsoleWindow.cpp
)

add_library(${MODULE_NAME}
	${MODULE_LIB}
	${MODULE_INCLUDE}
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDCoreLibs
	LDEditorContext
    LDEditorWidget
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
)

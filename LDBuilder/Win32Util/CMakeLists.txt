set(MODULE_NAME LDWin32Util)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensBuilder/Win32Util/Win32Util.h
)

set(MODULE_LIB
	Lib/Win32Util.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
	LDCoreLibs
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_BUILDER_MODULE_FOLDER})

set(MODULE_NAME LDLudensLFS)

set(MODULE_INCLUDE
	Include/LudensLFS.h
)

set(MODULE_LIB
	Lib/LudensLFS.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${LUDENS_SOURCE_DIR}
)

set(MODULE_NAME LDSystem)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/System/Memory.h
)

set(MODULE_LIB
	Lib/Memory.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${glfw_SOURCE_DIR}/include
)

target_link_libraries(${MODULE_NAME} PRIVATE
	glfw
)
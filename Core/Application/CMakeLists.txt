set(MODULE_NAME LDApplication)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/Application/Application.h
)

set(MODULE_LIB
	Lib/Application.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDSystem
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${glfw_SOURCE_DIR}/include
)

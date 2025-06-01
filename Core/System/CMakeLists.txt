set(MODULE_NAME LDSystem)
set(MODULE_TEST_NAME LDSystemTest)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/System/Memory.h
	${LUDENS_INCLUDE_DIR}/Ludens/System/Allocator.h
	${LUDENS_INCLUDE_DIR}/Ludens/System/Timer.h
	${LUDENS_INCLUDE_DIR}/Ludens/System/FileSystem.h
)

set(MODULE_LIB
	Lib/Memory.cpp
	Lib/Allocator.cpp
	Lib/Timer.cpp
	Lib/FileSystem.cpp
)

set(MODULE_TEST
	Test/SystemTest.cpp
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

add_executable(${MODULE_TEST_NAME}
	${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
	${MODULE_NAME}
)

target_include_directories(${MODULE_TEST_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${LUDENS_SOURCE_DIR}
)

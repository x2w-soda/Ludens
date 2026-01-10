set(MODULE_NAME LDSystem)
set(MODULE_TEST_NAME LDSystemTest)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/System/Timer.h
	${LUDENS_INCLUDE_DIR}/Ludens/System/FileSystem.h
	${LUDENS_INCLUDE_DIR}/Ludens/System/FileSystemAsync.h
	${LUDENS_INCLUDE_DIR}/Ludens/System/FileWatcher.h
	${LUDENS_INCLUDE_DIR}/Ludens/System/DropManager.h
)

set(MODULE_LIB
	Lib/Timer.cpp
	Lib/FileSystem.cpp
	Lib/FileSystemAsync.cpp
	Lib/FileWatcherWin32.cpp
	Lib/FileWatcherLinux.cpp
	Lib/DropManagerWin32.cpp
)

set(MODULE_TEST
	Test/SystemTest.cpp
	Test/FileSystemTest.cpp
	Test/FileSystemAsyncTest.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${glfw_SOURCE_DIR}/include
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDProfiler
	LDMemory
	LDDSA
	glfw
)

add_executable(${MODULE_TEST_NAME}
	${MODULE_TEST}
)

set_target_properties(${MODULE_TEST_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
	${MODULE_NAME}
	LDDSA
	LDLudensLFS
)

target_include_directories(${MODULE_TEST_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${LUDENS_SOURCE_DIR}
)

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

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_include_directories(${MODULE_NAME} SYSTEM PRIVATE
	${glfw_SOURCE_DIR}/include
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDProfiler
	LDMemory
	LDDSA
	glfw
)

add_ludens_core_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
	${MODULE_NAME}
	LDDSA
	LDLudensLFS
)

set(MODULE_NAME LDSystem)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/System/Timer.h
	${LUDENS_INCLUDE_DIR}/Ludens/System/FileSystem.h
	${LUDENS_INCLUDE_DIR}/Ludens/System/FileWatcher.h
	${LUDENS_INCLUDE_DIR}/Ludens/System/DropManager.h
)

set(MODULE_LIB
	Lib/Timer.cpp
	Lib/FileSystem.cpp
	Lib/FileWatcherWin32.cpp
	Lib/FileWatcherLinux.cpp
	Lib/DropManagerWin32.cpp
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
	glfw
)
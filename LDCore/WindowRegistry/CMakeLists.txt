set(MODULE_NAME LDWindowRegistry)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/WindowRegistry/WindowRegistry.h
	${LUDENS_INCLUDE_DIR}/Ludens/WindowRegistry/Input.h
)

set(MODULE_LIB
	Lib/Input.cpp
	Lib/Window.h
	Lib/Window.cpp
	Lib/WindowRegistry.cpp
	Lib/WindowWin32.cpp
    Lib/WindowLinux.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDSystem
	LDLog
	LDProfiler
	LDEvent
	LDMedia
)

target_include_directories(${MODULE_NAME} PRIVATE
	${glfw_SOURCE_DIR}/include
)

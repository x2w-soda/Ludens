set(MODULE_NAME LDApplication)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/Application/Application.h
	${LUDENS_INCLUDE_DIR}/Ludens/Application/Input.h
	${LUDENS_INCLUDE_DIR}/Ludens/Application/Event.h
)

set(MODULE_LIB
	Lib/Application.cpp
	Lib/Input.cpp
	Lib/InputInternal.h
	Lib/Window.h
	Lib/Window.cpp
	Lib/WindowWin32.cpp
    Lib/WindowLinux.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDSystem
	LDLog
	LDProfiler
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${glfw_SOURCE_DIR}/include
)

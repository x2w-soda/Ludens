set(MODULE_NAME LDWindow)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/Window/Window.h
	${LUDENS_INCLUDE_DIR}/Ludens/Window/Input.h
)

set(MODULE_LIB
	Lib/Input.cpp
	Lib/InputInternal.h
	Lib/WindowObj.h
	Lib/Window.cpp
	Lib/WindowWin32.cpp
    Lib/WindowLinux.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_link_libraries(${MODULE_NAME} PRIVATE
	LDSystem
	LDLog
	LDProfiler
	LDEvent
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${glfw_SOURCE_DIR}/include
)

set(MODULE_INCLUDE
	"Include/Application.h"
	"Include/ApplicationLayer.h"
	"Include/Event.h"
	"Include/Input.h")

set(MODULE_LIB
	"Lib/Application.cpp"
	"Lib/ApplicationWindow.h"
	"Lib/ApplicationWindow.cpp"
	"Lib/Event.cpp"
	"Lib/Input.cpp")

set(MODULE_INCLUDE_DIR
	"${CMAKE_SOURCE_DIR}/Ludens"
	"${CMAKE_SOURCE_DIR}/Extra/glfw/include")

add_library(LDApplication STATIC "${MODULE_LIB}" "${MODULE_INCLUDE}")
target_include_directories(LDApplication PRIVATE "${MODULE_INCLUDE_DIR}")
target_link_libraries(LDApplication PRIVATE glfw)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Lib" FILES "${MODULE_LIB}")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Include" FILES "${MODULE_INCLUDE}")
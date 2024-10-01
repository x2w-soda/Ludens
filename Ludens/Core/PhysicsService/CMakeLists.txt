set(MODULE_INCLUDE
	"Include/PhysicsService.h"
)

set(MODULE_LIB
	"Lib/PhysicsService.cpp"
)

set(MODULE_INCLUDE_DIR
	"${CMAKE_SOURCE_DIR}/Ludens"
)

add_library(LDPhysicsService STATIC
	"${MODULE_INCLUDE}"
	"${MODULE_LIB}"
)

target_link_libraries(LDPhysicsService PRIVATE
	LDPhysicsBase
)

target_include_directories(LDPhysicsService PRIVATE
	"${MODULE_INCLUDE_DIR}"
)

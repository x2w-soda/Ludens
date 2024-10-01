set(MODULE_INCLUDE
	"Include/Jolt/JoltTypes.h"
	"Include/Jolt/JoltPhysics.h"
	"Include/Jolt/JoltJobSystem.h"
	"Include/Jolt/JoltPhysicsSystem.h"
	"Include/Jolt/JoltBody.h"
	"Include/Jolt/JoltShape.h"
	"Include/PTypes.h"
)

set(MODULE_LIB
	"Lib/Jolt/JoltPhysics.cpp"
	"Lib/Jolt/JoltJobSystem.cpp"
	"Lib/Jolt/JoltPhysicsSystem.cpp"
	"Lib/Jolt/JoltBody.cpp"
	"Lib/Jolt/JoltShape.cpp"
	"Lib/PTypes.cpp"
)

set(MODULE_INCLUDE_DIR
	"${CMAKE_SOURCE_DIR}/Ludens"
)

add_library(LDPhysicsBase STATIC
	"${MODULE_INCLUDE}"
	"${MODULE_LIB}"
)

target_link_libraries(LDPhysicsBase PRIVATE
	Jolt
)

target_include_directories(LDPhysicsBase PRIVATE
	"${MODULE_INCLUDE_DIR}"
)

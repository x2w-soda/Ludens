set(MODULE_INCLUDE
	"Include/Memory.h"
	"Include/Allocator.h"
	"Include/Thread.h"
	"Include/Mutex.h"
	"Include/ConditionVariable.h"
	"Include/Exit.h"
	"Include/Time.h"
	"Include/UID.h"
	"Include/JobSystem.h"
)

set(MODULE_LIB
	"Lib/Memory.cpp"
	"Lib/Thread.cpp"
	"Lib/Mutex.cpp"
	"Lib/ConditionVariable.cpp"
	"Lib/Exit.cpp"
	"Lib/Time.cpp"
	"Lib/JobSystem.cpp"
)

set(TEST_SRC
	"Tests/TestPoolAllocator.h"
	"Tests/TestStackAllocator.h"
	"Tests/TestMemory.h"
	"Tests/TestUID.h"
	"Tests/OSTests.cpp"
)

set(MODULE_INCLUDE_DIR
	"${CMAKE_SOURCE_DIR}/Ludens"
)

add_library(LDOS STATIC
	"${MODULE_INCLUDE}"
	"${MODULE_LIB}"
)

target_include_directories(LDOS PRIVATE
	"${MODULE_INCLUDE_DIR}"
)

add_executable(LDOSTests
	"${MODULE_INCLUDE}"
	"${MODULE_LIB}"
	"${TEST_SRC}"
)

target_include_directories(LDOSTests PRIVATE
	"${CMAKE_SOURCE_DIR}/Ludens"
	"${CMAKE_SOURCE_DIR}/Extra/doctest"
)
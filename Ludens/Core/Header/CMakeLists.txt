set(MODULE_INCLUDE
	"Include/Platform.h"
	"Include/Singleton.h"
	"Include/Observer.h"
	"Include/Error.h"
	"Include/Types.h"
)

set(TEST_SRC
	"Tests/HeaderTests.cpp"
	"Tests/TestSingleton.h"
	"Tests/TestObserver.h"
)

add_executable(LDHeaderTests
	"${MODULE_INCLUDE}"
	"${TEST_SRC}"
)

target_include_directories(LDHeaderTests PRIVATE
	"${CMAKE_SOURCE_DIR}/Ludens"
	"${CMAKE_SOURCE_DIR}/Extra/doctest"
)
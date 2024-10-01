set(MODULE_INCLUDE
	"Include/CommandLine.h"
)

set(MODULE_LIB
	"Lib/CommandLine.cpp"
)

set(MODULE_TEST
	"Tests/TestCommandLine.cpp"
	"Tests/TestSingleArgument.h"
	"Tests/TestArguments.h"
)

set(MODULE_INCLUDE_DIR
	"${CMAKE_SOURCE_DIR}/Ludens"
)

add_library(LDCommandLine STATIC
	"${MODULE_LIB}"
	"${MODULE_INCLUDE}"
)

add_executable(LDCommandLineTests
	"${MODULE_TEST}"
)

target_link_libraries(LDCommandLineTests PRIVATE
	LDCommandLine
)
set(MODULE_LIB
	Lib/BuilderMain.h
	Lib/Shaderc.h
	Lib/Shaderc.cpp
)

set(MODULE_TEST
	Tests/BuilderTests.cpp
	Tests/TestShaderc.h
)

set(BUILDER_DEPENDENCIES
	LDOS
	LDIO
	LDApplication
	LDRenderBase
	LDRenderFX
	LDCommandLine
)

add_executable(LudensBuilder
	"${MODULE_LIB}"
	Lib/BuilderMain.cpp
)

target_link_libraries(LudensBuilder PRIVATE
	"${BUILDER_DEPENDENCIES}"
)

add_executable(LudensBuilderTests
	"${MODULE_LIB}"
	"${MODULE_TEST}"
)

target_link_libraries(LudensBuilderTests PRIVATE
	"${BUILDER_DEPENDENCIES}"
)
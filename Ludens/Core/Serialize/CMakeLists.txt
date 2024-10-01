set(MODULE_LIB
	"Lib/JSON.cpp"
	"Lib/INI.cpp"
	"Lib/XML.cpp"
	"Lib/MD.cpp"
)

set(MODULE_INCLUDE
	"Include/JSON.h"
	"Include/INI.h"
	"Include/XML.h"
	"Include/MD.h"
)

set(MODULE_INCLUDE_DIR
	"${CMAKE_SOURCE_DIR}/Ludens"
	"${CMAKE_SOURCE_DIR}/Extra/md4c/src"
	"${CMAKE_SOURCE_DIR}/Extra/doctest"
)

add_library(LDSerialize STATIC
	"${MODULE_LIB}"
	"${MODULE_INCLUDE}"
)

target_include_directories(LDSerialize PRIVATE
	"${MODULE_INCLUDE_DIR}"
)

target_link_libraries(LDSerialize PRIVATE
	md4c
)

set(TEST_SRC
	"Tests/SerializeTests.cpp"
	"Tests/SerializeINI.h"
	"Tests/SerializeXML.h"
	"Tests/SerializeMD.h"
)

add_executable(LDSerializeTests
	"${TEST_SRC}"
)

target_include_directories(LDSerializeTests PRIVATE
	"${MODULE_INCLUDE_DIR}"
)

target_link_libraries(LDSerializeTests PRIVATE
	LDSerialize
	LDOS
)
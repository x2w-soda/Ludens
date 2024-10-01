set(MODULE_SRC
	"Include/Array.h"
	"Include/Buffer.h"
	"Include/View.h"
	"Include/Stack.h"
	"Include/Vector.h"
	"Include/String.h"
	"Include/Optional.h"
)

set(TEST_SRC
	"Tests/TestBuffer.h"
	"Tests/TestStack.h"
	"Tests/TestVector.h"
	"Tests/TestString.h"
	"Tests/TestStringHash.h"
	"Tests/TestArray.h"
	"Tests/TestOptional.h"
	"Tests/DSATests.h"
	"Tests/DSATests.cpp"
)

set(BENCH_SRC
	"Benches/Bench.cpp"
)

add_executable(LDDSATests
	"${MODULE_SRC}"
	"${TEST_SRC}"
)

target_include_directories(LDDSATests PRIVATE
	"${CMAKE_SOURCE_DIR}/Ludens"
	"${CMAKE_SOURCE_DIR}/Extra/doctest"
)

target_link_libraries(LDDSATests LDOS)

add_executable(LDDSABenches
	"${MODULE_SRC}"
	"${BENCH_SRC}"
)

target_include_directories(LDDSABenches PRIVATE
	"${CMAKE_SOURCE_DIR}/Ludens"
)

target_link_libraries(LDDSABenches LDOS)
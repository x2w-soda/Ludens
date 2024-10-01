set(MODULE_SRC
	"Include/Math.h"
	"Include/Hash.h"
	"Include/Bits.h"
	"Include/Hex.h"
	"Include/Vec2.h"
	"Include/Vec3.h"
	"Include/Vec4.h"
	"Include/Mat3.h"
	"Include/Mat4.h"
	"Include/Quat.h"
	"Include/Rect2D.h"
)

set(TEST_SRC
	"Tests/TestBits.h"
	"Tests/TestVec2.h"
	"Tests/TestVec3.h"
	"Tests/TestVec4.h"
	"Tests/TestMat3.h"
	"Tests/TestMat4.h"
	"Tests/TestQuat.h"
	"Tests/MathTests.cpp")

add_executable(LDMathTests
	"${MODULE_SRC}"
	"${TEST_SRC}"
)

target_include_directories(LDMathTests PRIVATE
	"${CMAKE_SOURCE_DIR}/Ludens"
	"${CMAKE_SOURCE_DIR}/Extra/doctest"
)
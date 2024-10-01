set(MODULE_INCLUDE
	"Include/UI.h"
	"Include/UITheme.h"
	"Include/UILayout.h"
	"Include/UIWidget.h"
	"Include/UILogicStack.h"
	"Include/Container/Container.h"
	"Include/Container/UIScroll.h"
	"Include/Control/Control.h"
	"Include/Control/UIPanel.h"
	"Include/Control/UILabel.h"
	"Include/Control/UIButton.h"
	"Include/Control/UITexture.h"
)

set(MODULE_LIB
	"Lib/UIFont.cpp"
	"Lib/UITheme.cpp"
	"Lib/UIContext.cpp"
	"Lib/UILayout.cpp"
	"Lib/UIWidget.cpp"
	"Lib/UIWindow.cpp"
	"Lib/Container/UIScroll.cpp"
	"Lib/Control/UIPanel.cpp"
	"Lib/Control/UILabel.cpp"
	"Lib/Control/UIButton.cpp"
	"Lib/Control/UITexture.cpp"
)

set(MODULE_INCLUDE_DIR
	"${CMAKE_SOURCE_DIR}/Ludens"
	"${CMAKE_SOURCE_DIR}/Extra/yoga"
)

add_library(LDUI STATIC
	"${MODULE_LIB}"
	"${MODULE_INCLUDE}"
)

target_include_directories(LDUI PRIVATE
	"${MODULE_INCLUDE_DIR}"
)

target_link_libraries(LDUI PRIVATE
	yogacore
)

set(TEST_SRC
	"Tests/TestButton.h"
	"Tests/UITests.cpp"
)

add_executable(LDUITests
	"${TEST_SRC}"
)

target_include_directories(LDUITests PRIVATE
	"${CMAKE_SOURCE_DIR}/Ludens"
	"${CMAKE_SOURCE_DIR}/Extra/doctest"
)

target_link_libraries(LDUITests PRIVATE
	LDUI
)


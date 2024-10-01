set(MODULE_INCLUDE
	"Include/FileSystem.h")

set(MODULE_LIB
	"Lib/FileSystem.cpp")

set(MODULE_INCLUDE_DIR
	"${CMAKE_SOURCE_DIR}/Ludens")

add_library(LDIO STATIC
	"${MODULE_LIB}"
	"${MODULE_INCLUDE}")

target_include_directories(LDIO PRIVATE
	"${MODULE_INCLUDE_DIR}")

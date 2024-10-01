set(MODULE_INCLUDE
	"Include/Font.h"
	"Include/Image.h"
	"Include/Model.h"
	"Include/Mesh.h"
)

set(MODULE_LIB
	"Lib/Font.cpp"
	"Lib/Image.cpp"
	"Lib/Model.cpp"
	"Lib/ModelOBJ.h"
	"Lib/ModelOBJ.cpp"
	"Lib/ModelGLTF.h"
	"Lib/ModelGLTF.cpp"
	"Lib/Mesh.cpp"
)

set(MODULE_INCLUDE_DIR
	"${CMAKE_SOURCE_DIR}/Ludens"
	"${CMAKE_SOURCE_DIR}/Extra"
)

add_library(LDMedia STATIC
	"${MODULE_INCLUDE}"
	"${MODULE_LIB}"
)

target_include_directories(LDMedia PRIVATE
	"${MODULE_INCLUDE_DIR}"
)

target_link_libraries(LDMedia PRIVATE
	tinyobj
	tinygltf
	stb
)
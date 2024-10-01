add_library(tinygltf STATIC
	tiny_gltf.h
	tiny_gltf.cpp
)

target_include_directories(tinygltf PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/../rapidjson/include"
)
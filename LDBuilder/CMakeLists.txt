set(BUILDER_NAME LDBuilder)

set(BUILDER_SOURCE
	Main/Main.cpp
	Main/AudioUtil.h
	Main/AudioUtil.cpp
	Main/RenderUtil.h
	Main/RenderUtil.cpp
	Main/MeshUtil.h
	Main/MeshUtil.cpp
	Main/RunTests.h
	Main/RunTests.cpp
)

add_executable(${BUILDER_NAME}
	${BUILDER_SOURCE}
)

target_link_libraries(${BUILDER_NAME} PRIVATE
	LDCore
	LDBuilderLibs
)

target_include_directories(${BUILDER_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
)
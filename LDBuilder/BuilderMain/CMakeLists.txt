set(MODULE_NAME LDBuilder)

set(MODULE_LIB
	Lib/BuilderMain.cpp
	Lib/RunTests.h
	Lib/RunTests.cpp
)

add_executable(${MODULE_NAME}
	${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDCoreLibs
	LDBuilderLibs
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_BUILDER_MODULE_FOLDER})
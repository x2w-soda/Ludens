set(MODULE_NAME LDJobSystem)
set(MODULE_TEST_NAME LDJobSystemTest)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/JobSystem/JobSystem.h
)

set(MODULE_LIB
	Lib/JobSystem.cpp
)

set(MODULE_TEST
	Test/JobSystemTest.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
	LDSystem
	LDProfiler
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
)

add_executable(${MODULE_TEST_NAME}
	${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
	${MODULE_NAME}
)

target_include_directories(${MODULE_TEST_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${LUDENS_SOURCE_DIR}
)
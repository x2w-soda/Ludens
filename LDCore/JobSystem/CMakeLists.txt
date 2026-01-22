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

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
	LDSystem
	LDProfiler
)

add_ludens_core_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
	${MODULE_NAME}
)
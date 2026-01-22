set(MODULE_NAME LDCommandLine)
set(MODULE_TEST_NAME LDCommandLineTest)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/CommandLine/ArgParser.h
)

set(MODULE_LIB
    Lib/ArgParser.cpp
)

set(MODULE_TEST
    Test/CommandLineTest.cpp
    Test/TestArgParser.h
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDSystem
)

add_ludens_core_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
)
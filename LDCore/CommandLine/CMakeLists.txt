set(MODULE_NAME LDCommandLine)
set(MODULE_TEST_NAME LDCommandLineTest)

set(MODULE_INCLUDE
  "${LUDENS_INCLUDE_DIR}/Ludens/CommandLine/ArgParser.h"
)

set(MODULE_LIB
    Lib/ArgParser.cpp
)

set(MODULE_TEST
    Test/CommandLineTest.cpp
    Test/TestArgParser.h
)

add_library(${MODULE_NAME}
    ${MODULE_INCLUDE}
    ${MODULE_LIB}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDSystem
)

add_executable(${MODULE_TEST_NAME}
    ${MODULE_TEST}
)

set_target_properties(${MODULE_TEST_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_include_directories(${MODULE_TEST_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_SOURCE_DIR}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
)
set(MODULE_NAME LDUI)
set(MODULE_TEST_NAME LDUITest)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/UI/UI.h
)

set(MODULE_LIB
    Lib/UI.cpp
)

set(MODULE_TEST
    Test/UITest.cpp
)

add_library(${MODULE_NAME}
    ${MODULE_INCLUDE}
    ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDProfiler
    LDSystem
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_SOURCE_DIR}
)

add_executable(${MODULE_TEST_NAME}
    ${MODULE_TEST}
)

target_include_directories(${MODULE_TEST_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_SOURCE_DIR}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
)
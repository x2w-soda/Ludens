set(MODULE_NAME LDDocumentBuilder)
set(MODULE_TEST_NAME LDDocumentBuilderTest)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/DocumentBuilder/Document.h
	${LUDENS_INCLUDE_DIR}/LudensBuilder/DocumentBuilder/DocumentBuilder.h
)

set(MODULE_LIB
    Lib/Document.cpp
	Lib/DocumentBuilder.cpp
)

set(MODULE_TEST
    Test/DocumentBuilderTest.cpp
    Test/DocumentItemTest.cpp
    Test/DocumentSpanTest.cpp
)

add_ludens_builder_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

add_ludens_builder_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
)
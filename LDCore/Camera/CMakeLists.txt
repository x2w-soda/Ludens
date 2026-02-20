set(MODULE_NAME LDCamera)
set(MODULE_TEST_NAME LDCameraTest)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/Camera/Camera.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Camera/CameraController.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Camera/Camera2D.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Camera/Camera2DController.h
)

set(MODULE_LIB
    Lib/Camera.cpp
    Lib/CameraController.cpp
    Lib/Camera2D.cpp
    Lib/Camera2DController.cpp
    Lib/CameraCommon.h
)

set(MODULE_TEST
    Test/CameraTest.cpp
    Test/Camera2DTest.cpp
    Test/PerspectiveTest.cpp
    Test/OrthographicTest.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

add_ludens_core_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
    LDSystem
)
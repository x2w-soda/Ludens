set(MODULE_NAME LDCamera)
set(MODULE_TEST_NAME LDCameraTest)

set(MODULE_INCLUDE
  "${LUDENS_INCLUDE_DIR}/Ludens/Camera/Camera.h"
  "${LUDENS_INCLUDE_DIR}/Ludens/Camera/CameraController.h"
)

set(MODULE_LIB
    Lib/Camera.cpp
    Lib/CameraController.cpp
    Lib/CameraCommon.h
)

set(MODULE_TEST
    Test/CameraTest.cpp
    Test/TestPerspective.h
    Test/TestOrthographic.h
)

add_library(${MODULE_NAME}
    ${MODULE_INCLUDE}
    ${MODULE_LIB}
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
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
    LDSystem
)
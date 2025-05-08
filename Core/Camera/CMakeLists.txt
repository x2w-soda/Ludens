set(MODULE_NAME LDCamera)

set(MODULE_INCLUDE
  "${LUDENS_INCLUDE_DIR}/Ludens/Camera/Camera.h"
  "${LUDENS_INCLUDE_DIR}/Ludens/Camera/CameraController.h"
)

set(MODULE_LIB
    Lib/Camera.cpp
    Lib/CameraController.cpp
    Lib/CameraCommon.h
)

add_library(${MODULE_NAME}
    ${MODULE_INCLUDE}
    ${MODULE_LIB}
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
)
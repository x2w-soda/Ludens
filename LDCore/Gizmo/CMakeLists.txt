set(MODULE_NAME LDGizmo)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/Gizmo/Gizmo.h
)

set(MODULE_LIB
    Lib/Gizmo.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDCamera
)
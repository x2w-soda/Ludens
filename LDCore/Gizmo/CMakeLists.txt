set(MODULE_NAME LDGizmo)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/Gizmo/Gizmo.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Gizmo/Gizmo2D.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Gizmo/GizmoEnum.h
)

set(MODULE_LIB
    Lib/Gizmo.cpp
    Lib/Gizmo2D.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDCamera
)
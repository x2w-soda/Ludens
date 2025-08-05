set(MODULE_NAME LDGizmo)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/Gizmo/Gizmo.h
)

set(MODULE_LIB
    Lib/Gizmo.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDCamera
)
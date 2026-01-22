set(MODULE_NAME LDMeshUtil)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensBuilder/MeshUtil/MeshUtil.h
)

set(MODULE_LIB
	Lib/MeshUtil.cpp
)

add_ludens_builder_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)
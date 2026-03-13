set(MODULE_NAME LDProjectBuilder)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensBuilder/ProjectBuilder/ProjectBuilder.h
    ${LD_EMBED_RUNTIME_H}
)

set(MODULE_LIB
	Lib/ProjectBuilder.cpp
    ${LD_EMBED_RUNTIME_CPP}
)

add_ludens_builder_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_BUILD_EMBED_DIR}
)

add_dependencies(${MODULE_NAME} LDEmbedRuntime)
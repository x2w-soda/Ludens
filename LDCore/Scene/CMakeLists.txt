set(MODULE_NAME LDScene)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/Scene/Scene.h
)

set(MODULE_LIB
    Lib/Scene.cpp
    Lib/LuaScript.h
    Lib/LuaScript.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDProfiler
    LDSystem
    LDLua
    LDDataRegistry
)

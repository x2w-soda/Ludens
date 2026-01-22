set(MODULE_NAME LDAudioUtil)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensBuilder/AudioUtil/AudioUtil.h
)

set(MODULE_LIB
	Lib/AudioUtil.cpp
)

add_ludens_builder_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)
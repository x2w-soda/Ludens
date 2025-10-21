set(MODULE_NAME LDDSP)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/DSP/DSP.h
    ${LUDENS_INCLUDE_DIR}/Ludens/DSP/Resampler.h
    ${LUDENS_INCLUDE_DIR}/Ludens/DSP/BiquadFilterCoeff.h
)

set(MODULE_LIB
    Lib/DSP.cpp
    Lib/Resampler.cpp
    Lib/BiquadFilterCoeff.cpp
)

add_library(${MODULE_NAME}
    ${MODULE_INCLUDE}
    ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDSystem
    LDProfiler
    SampleRate::samplerate
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
    ${samplerate_SOURCE_DIR}/include
)

set(MODULE_NAME LDRenderBackend)
set(MODULE_TEST_NAME LDRenderBackendTest)

set(LUDENS_VULKAN_SDK_LIBS
    debug OSDependentd optimized OSDependent
    debug MachineIndependentd optimized MachineIndependent
    debug GenericCodeGend optimized GenericCodeGen
    debug glslangd optimized glslang
    debug SPIRVd optimized SPIRV
    debug SPIRV-Toolsd optimized SPIRV-Tools
    debug SPIRV-Tools-optd optimized SPIRV-Tools-opt
    debug glslang-default-resource-limitsd optimized glslang-default-resource-limits
    debug spirv-cross-cored optimized spirv-cross-core
    debug spirv-cross-glsld optimized spirv-cross-glsl
    debug spirv-cross-reflectd optimized spirv-cross-reflect
)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/RenderBackend/RBackend.h
    ${LUDENS_INCLUDE_DIR}/Ludens/RenderBackend/RBackendEnum.h
    ${LUDENS_INCLUDE_DIR}/Ludens/RenderBackend/RUtil.h
    ${LUDENS_INCLUDE_DIR}/Ludens/RenderBackend/RStager.h
    ${LUDENS_INCLUDE_DIR}/Ludens/RenderBackend/LDShaderParser.h
    ${LUDENS_INCLUDE_DIR}/Ludens/RenderBackend/LDShaderCompiler.h
    ${LUDENS_INCLUDE_DIR}/Ludens/RenderBackend/KTX.h
)

set(MODULE_LIB
    Lib/RData.h
    Lib/RCommand.h
    Lib/RCommand.cpp
    Lib/RBackendObj.h
    Lib/RBackendVK.cpp
    Lib/RBackendGL.cpp
    Lib/RBackend.cpp
    Lib/RShaderCompiler.h
    Lib/RShaderCompiler.cpp
    Lib/RUtilCommon.h
    Lib/RUtilVK.h
    Lib/RUtilGL.h
    Lib/RUtil.cpp
    Lib/RStager.cpp
    Lib/LDShaderParser.cpp
    Lib/LDShaderCompiler.cpp
    Lib/KTX.cpp
    ${LUDENS_EXTRA_DIR}/glad/src/glad.c
)

set(MODULE_TEST
    Test/RBackendTest.h
    Test/RBackendTest.cpp
    Test/RBackendShaderParserTest.cpp
    Test/RBackendPrimitiveTest.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_directories(${MODULE_NAME} PUBLIC
    ${LUDENS_VULKAN_LIB_DIR}
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${Vulkan_INCLUDE_DIR}
    ${glfw_SOURCE_DIR}/include
    ${vma_SOURCE_DIR}/include
    ${ktxsoftware_SOURCE_DIR}/include
    ${LUDENS_EXTRA_DIR}/glad/include
)

target_link_libraries(${MODULE_NAME} PUBLIC
    ${LUDENS_VULKAN_SDK_LIBS}
    ${Vulkan_LIBRARIES}
    LDProfiler
    LDLog
    LDWindowRegistry
)

add_ludens_core_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
    LDSystem
    LDMedia
)

set(MODULE_TEST_NAME LDHeaderTest)
set(MODULE_BENCHMARK_NAME LDHeaderBenchmark)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Assert.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Bitwise.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Class.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Handle.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Platform.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/SIMD.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Math/Math.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Math/Vec2.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Math/Vec3.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Math/Vec4.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Math/Quat.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Math/Mat3.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Math/Mat4.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Math/Rect.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Math/Geometry.h
)

set(MODULE_TEST
    ${MODULE_INCLUDE}
    Test/HeaderTest.cpp
    Test/TestHash.h
    Test/TestGeometry.h
)

add_executable(${MODULE_TEST_NAME}
    ${MODULE_TEST}
)

target_include_directories(${MODULE_TEST_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_SOURCE_DIR}
)

if (LD_BUILD_BENCHMARKS)
    add_executable(${MODULE_BENCHMARK_NAME}
        ${MODULE_INCLUDE}
        Benchmark/simd.cpp
    )
    target_include_directories(${MODULE_BENCHMARK_NAME} PRIVATE
        ${LUDENS_INCLUDE_DIR}
        ${LUDENS_SOURCE_DIR}
    )
    target_link_libraries(${MODULE_BENCHMARK_NAME} PRIVATE
        LDSystem
    )
endif()
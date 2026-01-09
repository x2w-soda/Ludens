set(MODULE_NAME LDMemory)
set(MODULE_TEST_NAME LDMemoryTest)
set(MODULE_BENCH_NAME LDMemoryBench)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/Memory/Memory.h
	${LUDENS_INCLUDE_DIR}/Ludens/Memory/Allocator.h
)

set(MODULE_LIB
	Lib/Memory.cpp
	Lib/Allocator.cpp
)

set(MODULE_TEST
	Test/MemoryTest.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${glfw_SOURCE_DIR}/include
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDProfiler
	glfw
)

add_executable(${MODULE_TEST_NAME}
	${MODULE_TEST}
)

set_target_properties(${MODULE_TEST_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
	${MODULE_NAME}
	LDProfiler
)

target_include_directories(${MODULE_TEST_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${LUDENS_SOURCE_DIR}
)

if (LD_BUILD_BENCHMARKS)
    add_executable(${MODULE_BENCH_NAME}
        Bench/MemoryBench.cpp
    )
	set_target_properties(${MODULE_BENCH_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})
    target_include_directories(${MODULE_BENCH_NAME} PRIVATE
        ${LUDENS_INCLUDE_DIR}
        ${LUDENS_SOURCE_DIR}
        ${entt_SOURCE_DIR}/src
    )
    target_link_libraries(${MODULE_BENCH_NAME} PRIVATE
        ${MODULE_NAME}
        LDSystem
    )
endif()
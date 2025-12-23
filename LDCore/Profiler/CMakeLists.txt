set(MODULE_NAME LDProfiler)

set(MODULE_INCLUDE
  "${LUDENS_INCLUDE_DIR}/Ludens/Profiler/Profiler.h"
)

set(MODULE_LIB
  Lib/Profiler.cpp
)

add_library(${MODULE_NAME} STATIC
  ${MODULE_INCLUDE}
  ${MODULE_LIB}
)

target_include_directories(${MODULE_NAME} PUBLIC
  ${LUDENS_INCLUDE_DIR}
  ${tracy_SOURCE_DIR}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

# see root CMakeLists.txt
if (LD_COMPILE_WITH_PROFILER)
  target_link_libraries(${MODULE_NAME} PUBLIC TracyClient)
  message(STATUS "LDProfiler linking with TracyClient")
else()
  message(STATUS "LDProfiler skipping TracyClient")
endif()
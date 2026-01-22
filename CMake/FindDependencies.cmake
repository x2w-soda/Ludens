# Find Vulkan
# - require SPIRV-Cross for reflection
# - require glslang for SPIRV compilation
find_package(Vulkan REQUIRED SPIRV-Tools)

set(LUDENS_VULKAN_INCLUDE_DIR ${Vulkan_INCLUDE_DIR})
set(LUDENS_VULKAN_LIB_DIR     ${Vulkan_INCLUDE_DIR}/../Lib)
message(STATUS "LUDENS Vulkan INCLUDE DIR:  ${LUDENS_VULKAN_INCLUDE_DIR}")
message(STATUS "LUDENS Vulkan LIB DIR:      ${LUDENS_VULKAN_LIB_DIR}")

# find Doxygen

find_package(Doxygen REQUIRED)
set(LUDENS_DOXYFILE_IN  "${LUDENS_SOURCE_DIR}/Doxyfile.in")
set(LUDENS_DOXYFILE_OUT "${CMAKE_BINARY_DIR}/Doxyfile")
set(LUDENS_DOXYGEN_DIR  "${CMAKE_BINARY_DIR}/Doc")
message(STATUS "LUDENS DOXFILE IN:          ${LUDENS_DOXYFILE_IN}")
message(STATUS "LUDENS DOXFILE OUT:         ${LUDENS_DOXYFILE_OUT}")
message(STATUS "LUDENS DOXGEN DIR:          ${LUDENS_DOXYGEN_DIR}")

set(LUDENS_DOXYGEN_INPUT "\"${LUDENS_SOURCE_DIR}/Core\" \"${LUDENS_SOURCE_DIR}/Include\"")

configure_file(${LUDENS_DOXYFILE_IN} ${LUDENS_DOXYFILE_OUT} @ONLY)

add_custom_target(ld_doxygen
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "generating documentation with Doxygen"
)

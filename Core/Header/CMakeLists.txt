set(MODULE_TEST_NAME LDHeaderTest)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Assert.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Bitwise.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Class.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Handle.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Platform.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Math/Math.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Math/Vec2.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Math/Vec3.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Math/Vec4.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Math/Quat.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Header/Math/Mat4.h
)

set(MODULE_TEST
    ${MODULE_INCLUDE}
    Test/HeaderTest.cpp
)

add_executable(${MODULE_TEST_NAME}
    ${MODULE_TEST}
)

target_include_directories(${MODULE_TEST_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_SOURCE_DIR}
)

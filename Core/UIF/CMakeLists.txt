set(MODULE_NAME LDUIF)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/UIF/UIFWindow.h
    ${LUDENS_INCLUDE_DIR}/Ludens/UIF/UIFWidget.h
    ${LUDENS_INCLUDE_DIR}/Ludens/UIF/UIFTheme.h
    ${LUDENS_INCLUDE_DIR}/Ludens/UIF/UIFWindowManager.h
    ${LUDENS_INCLUDE_DIR}/Ludens/UIF/UIFAnimation.h
)

set(MODULE_LIB
    Lib/UIFWindow.cpp
    Lib/UIFWidgetObj.h
    Lib/UIFWidget.cpp
    Lib/UIFTheme.cpp
    Lib/UIFWindowManager.cpp
)

add_library(${MODULE_NAME}
    ${MODULE_INCLUDE}
    ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDSystem
    LDUI
    LDRenderComponent
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
)
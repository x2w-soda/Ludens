set(MODULE_NAME LDViewportWindow)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensEditor/ViewportWindow/ViewportWindow.h
)

set(MODULE_LIB
    Lib/ViewportCommon.h
    Lib/ViewportWindow.cpp
    Lib/Viewport3D.h
    Lib/Viewport3D.cpp
    Lib/Viewport2D.h
    Lib/Viewport2D.cpp
)

add_ludens_editor_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDEditorContext
    LDEditorWidget
)

set(MODULE_NAME LDRenderComponent)

set(MODULE_INCLUDE
    "${LUDENS_INCLUDE_DIR}/Ludens/RenderComponent/ForwardRender.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/RenderComponent/ScreenRender.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/RenderComponent/ScreenPick.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/RenderComponent/Outline.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/RenderComponent/DualKawase.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/RenderComponent/Layout/SetLayouts.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/RenderComponent/Layout/VertexLayouts.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/RenderComponent/Layout/PipelineLayouts.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/RenderComponent/Layout/RMaterial.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/RenderComponent/Layout/RMesh.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/RenderComponent/Pipeline/RMeshPipeline.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/RenderComponent/Pipeline/LinePipeline.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/RenderComponent/Pipeline/SkyboxPipeline.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/RenderComponent/Pipeline/OutlinePipeline.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/RenderComponent/Pipeline/EquirectangularPipeline.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/RenderComponent/Embed/GizmoMesh.h"
)

set(MODULE_LIB
    "Lib/ForwardRender.cpp"
    "Lib/Outline.cpp"
    "Lib/ScreenRender.cpp"
    "Lib/ScreenPick.cpp"
    "Lib/DualKawase.cpp"
    "Lib/Layout/SetLayouts.cpp"
    "Lib/Layout/VertexLayouts.cpp"
    "Lib/Layout/PipelineLayouts.cpp"
    "Lib/Layout/RMesh.cpp"
    "Lib/Pipeline/RMeshPipeline.cpp"
    "Lib/Pipeline/LinePipeline.cpp"
    "Lib/Pipeline/SkyboxPipeline.cpp"
    "Lib/Pipeline/OutlinePipeline.cpp"
    "Lib/Pipeline/EquirectangularPipeline.cpp"
    "Lib/Embed/GizmoMesh.cpp"
)

add_library(${MODULE_NAME}
    ${MODULE_INCLUDE}
    ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDProfiler
    LDSystem
    LDRenderBackend
    LDRenderGraph
)

target_include_directories(${MODULE_NAME} PRIVATE
  ${LUDENS_INCLUDE_DIR}
)
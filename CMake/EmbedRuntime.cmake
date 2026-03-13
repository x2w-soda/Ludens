set(LD_EMBED_RUNTIME_H "${LUDENS_BUILD_EMBED_DIR}/EmbedRuntime.h")
set(LD_EMBED_RUNTIME_CPP "${LUDENS_BUILD_EMBED_DIR}/EmbedRuntime.cpp")

set_source_files_properties(
    "${LD_EMBED_RUNTIME_H}"
    "${LD_EMBED_RUNTIME_CPP}"
    PROPERTIES GENERATED TRUE)

if (LD_OPTION_EMBED_RUNTIME)
    add_custom_command(
        OUTPUT "${LD_EMBED_RUNTIME_H}" "${LD_EMBED_RUNTIME_CPP}"
        COMMAND ${Python3_EXECUTABLE} -u "${LUDENS_SCRIPTS_DIR}/Embed.py" "$<TARGET_FILE:LDRuntime>" "EmbedRuntime" "${LUDENS_BUILD_EMBED_DIR}"
        DEPENDS LDRuntime
    )
else()
    add_custom_command(
        OUTPUT "${LD_EMBED_RUNTIME_H}" "${LD_EMBED_RUNTIME_CPP}"
        COMMAND ${CMAKE_COMMAND} -E echo "#pragma once" > "${LD_EMBED_RUNTIME_H}"
        COMMAND ${CMAKE_COMMAND} -E echo "#include <cstddef>" >> "${LD_EMBED_RUNTIME_H}"
        COMMAND ${CMAKE_COMMAND} -E echo "extern \"C\" { extern const unsigned char* EmbedRuntimeData; extern size_t EmbedRuntimeSize; }" >> "${LD_EMBED_RUNTIME_H}"
        COMMAND ${CMAKE_COMMAND} -E echo "#include <cstddef>" > "${LD_EMBED_RUNTIME_CPP}"
        COMMAND ${CMAKE_COMMAND} -E echo "extern \"C\" { const unsigned char* EmbedRuntimeData = nullptr; size_t EmbedRuntimeSize = 0; }" >> "${LD_EMBED_RUNTIME_CPP}"
        VERBATIM
    )
endif()

add_custom_target(LDEmbedRuntime DEPENDS
    ${LD_EMBED_RUNTIME_H}
    ${LD_EMBED_RUNTIME_CPP}
)

#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <vector>

#define LD_GLSL_VERSION 460
#define LD_GLSL_ENTRY_POINT "main"

namespace LD {

class RShaderCompiler
{
public:
    RShaderCompiler() = delete;
    RShaderCompiler(RDeviceBackend backend);

    bool compile(RShaderType type, const char* glsl, std::vector<uint32_t>& spirvCode);

private:
    RDeviceBackend mBackend;
};

} // namespace LD
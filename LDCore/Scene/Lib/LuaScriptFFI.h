#pragma once

#include <Ludens/Header/Platform.h>

#ifndef LD_FFI_EXPORT
#if defined(LD_PLATFORM_WIN32)
#define LD_FFI_EXPORT __declspec(dllexport)
#elif defined(LD_PLATFORM_LINUX)
#define LD_FFI_EXPORT __attribute__((visibility("default")))
#endif
#endif // LD_FFI_EXPORT

namespace LD {

struct AudioSourceComponent;

namespace LuaScript {

const char* get_ffi_cdef();
const char* get_ffi_mt();

extern "C" {
LD_FFI_EXPORT void ffi_audio_source_component_play(AudioSourceComponent* comp);
LD_FFI_EXPORT void ffi_audio_source_component_pause(AudioSourceComponent* comp);
LD_FFI_EXPORT void ffi_audio_source_component_resume(AudioSourceComponent* comp);
} // extern "C"
} // namespace LuaScript
} // namespace LD
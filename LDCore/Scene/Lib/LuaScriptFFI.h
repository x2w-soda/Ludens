#pragma once

#include <Ludens/Header/Platform.h>
#include <cstdint>

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
LD_FFI_EXPORT uint32_t ffi_get_parent_id(uint32_t compID);
LD_FFI_EXPORT uint32_t ffi_get_child_id_by_name(uint32_t compID, const char* name);
LD_FFI_EXPORT void ffi_audio_source_component_play(AudioSourceComponent* comp);
LD_FFI_EXPORT void ffi_audio_source_component_pause(AudioSourceComponent* comp);
LD_FFI_EXPORT void ffi_audio_source_component_resume(AudioSourceComponent* comp);
LD_FFI_EXPORT void ffi_audio_source_component_set_pan(AudioSourceComponent* comp, float pan);
LD_FFI_EXPORT void ffi_audio_source_component_set_volume_linear(AudioSourceComponent* comp, float volumeLinear);
} // extern "C"
} // namespace LuaScript
} // namespace LD
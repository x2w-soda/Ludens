#pragma once

// dragging third party headers and wrap with macros
#include <public/tracy/Tracy.hpp>

#define LD_PROFILE_SCOPE ZoneScoped
#define LD_PROFILE_SCOPE_NAME ZoneScopedN
#define LD_PROFILE_FRAME_MARK FrameMark
#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Project/Project.h>
#include <Ludens/RenderSystem/RenderSystem.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

struct RuntimeContextInfo
{
    Project project;
};

/// @brief Context for the game runtime.
struct RuntimeContext : Handle<struct RuntimeContextObj>
{
    /// @brief Create the runtime context.
    static RuntimeContext create(const RuntimeContextInfo& info);

    /// @brief Destroy the runtime context.
    static void destroy(RuntimeContext ctx);

    /// @brief Updates the scripts in current Scene.
    /// @param delta Delta time in seconds.
    void update(float delta);
};

} // namespace LD
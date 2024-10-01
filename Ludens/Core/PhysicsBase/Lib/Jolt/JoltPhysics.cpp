#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>
#include "Core/PhysicsBase/Include/Jolt/JoltPhysics.h"

namespace LD
{

namespace JoltPhysics
{

static bool sHasStartup;

void Startup()
{
    if (sHasStartup)
        return;

    JPH::RegisterDefaultAllocator();

	JPH::Factory::sInstance = new JPH::Factory();

    JPH::RegisterTypes();

    sHasStartup = true;
}

void Cleanup()
{
    if (!sHasStartup)
        return;

    sHasStartup = false;

    JPH::UnregisterTypes();

    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

} // namespace JoltPhysics

} // namespace LD
#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/TempAllocator.h>
#include "Core/Header/Include/Error.h"
#include "Core/OS/Include/Time.h"
#include "Core/PhysicsBase/Include/Jolt/JoltJobSystem.h"
#include "Core/PhysicsBase/Include/Jolt/JoltBody.h"

namespace LD
{

namespace JoltObjectLayer
{

static constexpr JPH::ObjectLayer Static = 0;
static constexpr JPH::ObjectLayer Dynamic = 1;
static constexpr unsigned int NUM_LAYERS = 2;

} // namespace JoltObjectLayer

class JoltObjectLayerPairFilter : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer lhs, JPH::ObjectLayer rhs) const override
    {
        switch (lhs)
        {
        case JoltObjectLayer::Static:
            return rhs == JoltObjectLayer::Dynamic;
        case JoltObjectLayer::Dynamic:
            return true;
        default:
            LD_DEBUG_UNREACHABLE;
        }

        return false;
    }
};

namespace JoltBroadPhaseLayers
{

static constexpr JPH::BroadPhaseLayer Static(0);
static constexpr JPH::BroadPhaseLayer Dynamic(1);
static constexpr unsigned int NUM_LAYERS(2);

} // namespace JoltBroadPhaseLayers

class JoltBroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface
{
public:
    JoltBroadPhaseLayerInterface()
    {
        mObjectToBPL[JoltObjectLayer::Static] = JoltBroadPhaseLayers::Static;
        mObjectToBPL[JoltObjectLayer::Dynamic] = JoltBroadPhaseLayers::Dynamic;
    }

    virtual JPH::uint GetNumBroadPhaseLayers() const override
    {
        return JoltBroadPhaseLayers::NUM_LAYERS;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
    {
        LD_DEBUG_ASSERT(layer < JoltObjectLayer::NUM_LAYERS);

        return mObjectToBPL[layer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
    {
        switch ((JPH::BroadPhaseLayer::Type)layer)
        {
        case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayers::Static:
            return "Static";
        case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayers::Dynamic:
            return "Dynamic";
        default:
            LD_DEBUG_UNREACHABLE;
        }

        return "INVALID";
    }
#endif

private:
    JPH::BroadPhaseLayer mObjectToBPL[JoltBroadPhaseLayers::NUM_LAYERS];
};

class JoltObjectVsBroadPhaseLayerFilter : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer lhs, JPH::BroadPhaseLayer rhs) const override
    {
        switch (lhs)
        {
        case JoltObjectLayer::Static:
            return rhs == JoltBroadPhaseLayers::Dynamic;
        case JoltObjectLayer::Dynamic:
            return true;
        default:
            LD_DEBUG_UNREACHABLE;
        }

        return false;
    }
};

class JoltPhysicsSystem
{
public:
    JoltPhysicsSystem();
    JoltPhysicsSystem(const JoltPhysicsSystem&) = delete;
    ~JoltPhysicsSystem();

    JoltPhysicsSystem& operator=(const JoltPhysicsSystem&) = delete;

    void Startup();
    void Cleanup();

    void Update(DeltaTime dt);

    JPH::BodyInterface& GetBodyInterface();

    const JPH::BodyLockInterface& GetBodyLockInterface() const;

    JPH::BodyID AddRigidBody(const JPH::BodyCreationSettings& settings);

    void RemoveBody(const JPH::BodyID& id);

    JoltBodyReadResult ReadBody(const JPH::BodyID& id) const;

    JoltBodyWriteResult WriteBody(const JPH::BodyID& id) const;

private:
    JPH::PhysicsSystem mPhysics;
    JPH::TempAllocatorImpl mTempAllocator;
    JoltJobSystem mJobSystem;
    JoltObjectLayerPairFilter mOjectLayerPairFilter;
    JoltBroadPhaseLayerInterface mBroadPhaseLayerInterface;
    JoltObjectVsBroadPhaseLayerFilter mObjectVsBroadPhaseLayerFilter;
};

} // namespace LD
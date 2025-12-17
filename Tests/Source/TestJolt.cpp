/**
 * @file TestJolt.cpp
 * @brief Smoke test to validate Jolt Physics (static) integration.
 */

#include "doctest.h"
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

using namespace JPH;

// Simple layer setup for testing
namespace Layers
{
    static constexpr ObjectLayer NON_MOVING = 0;
    static constexpr ObjectLayer MOVING = 1;
    static constexpr ObjectLayer NUM_LAYERS = 2;
};

namespace BroadPhaseLayers
{
    static constexpr BroadPhaseLayer NON_MOVING(0);
    static constexpr BroadPhaseLayer MOVING(1);
    static constexpr uint NUM_LAYERS(2);
};

// Minimal implementations for testing
class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }

    virtual uint GetNumBroadPhaseLayers() const override
    {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
    {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

private:
    BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
    {
        return true;
    }
};

class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
    {
        return true;
    }
};

TEST_SUITE("Jolt")
{
    TEST_CASE("Create physics system and step")
    {
        // Register allocation hook
        RegisterDefaultAllocator();

        // Create factory
        Factory::sInstance = new Factory();

        // Register all physics types
        RegisterTypes();

        // Create temp allocator
        TempAllocatorImpl temp_allocator(1024 * 1024); // 1 MB

        // Create job system
        JobSystemThreadPool job_system(cMaxPhysicsJobs, cMaxPhysicsBarriers, 1);

        // Create layer interfaces
        BPLayerInterfaceImpl broad_phase_layer_interface;
        ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
        ObjectLayerPairFilterImpl object_vs_object_layer_filter;

        // Create and initialize physics system
        PhysicsSystem physics_system;
        physics_system.Init(
            1024,  // max bodies
            0,     // num body mutexes (auto-detect)
            1024,  // max body pairs
            1024,  // max contact constraints
            broad_phase_layer_interface,
            object_vs_broadphase_layer_filter,
            object_vs_object_layer_filter
        );

        // Step the physics system once
        physics_system.Update(1.0f / 60.0f, 1, &temp_allocator, &job_system);

        // Cleanup
        UnregisterTypes();
        delete Factory::sInstance;
        Factory::sInstance = nullptr;
    }
}

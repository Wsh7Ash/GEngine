#pragma once

#include "../System.h"
#include "../../math/VecTypes.h"
#include "../../math/quaternion.h"

// Jolt includes - typically we might want to wrap these or use forward declarations to keep headers clean
// For the sake of this integration, we'll assume the system CPP will handle the heavy lifting.

namespace JPH {
    class PhysicsSystem;
    class JobSystemThreadPool;
    class TempAllocatorImpl;
    class BodyInterface;
    class ContactListener;
}

namespace ge {
namespace ecs {

    /**
     * @brief System that manages 3D physics simulation using Jolt Physics.
     */
    class Physics3DSystem : public System {
    public:
        Physics3DSystem();
        ~Physics3DSystem();

        void Update(World& world, float dt);
        
        // Internal body management
        void OnRigidbodyAdded(Entity entity, World& world);
        void OnRigidbodyRemoved(Entity entity, World& world);

    private:
        void InitializeJolt();
        void ShutdownJolt();

        // Jolt core objects
        JPH::PhysicsSystem* m_PhysicsSystem = nullptr;
        JPH::JobSystemThreadPool* m_JobSystem = nullptr;
        JPH::TempAllocatorImpl* m_TempAllocator = nullptr;
        JPH::ContactListener* m_ContactListener = nullptr;
        
        // Mappings for sync
        // In a real engine, we'd have a pool/manager for bodies.
    };

} // namespace ecs
} // namespace ge

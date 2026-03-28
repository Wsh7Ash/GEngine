#pragma once

#include "../System.h"
#include "../../math/VecTypes.h"
#include "../../math/quaternion.h"

namespace JPH {
    class PhysicsSystem;
    class JobSystemThreadPool;
    class TempAllocatorImpl;
    class BodyInterface;
    class ContactListener;
    class SoftBody;
    class SoftBodyInterface;
    class Constraint;
    class Body;
}

namespace ge {
namespace ecs {

    class SoftBody;

    /**
     * @brief System that manages 3D physics simulation using Jolt Physics.
     */
    class Physics3DSystem : public System {
    public:
        Physics3DSystem();
        ~Physics3DSystem();

        void Update(World& world, float dt);
        
        void OnRigidbodyAdded(Entity entity, World& world);
        void OnRigidbodyRemoved(Entity entity, World& world);

        JPH::PhysicsSystem* GetPhysicsSystem() const { return m_PhysicsSystem; }
        JPH::SoftBodyInterface* GetSoftBodyInterface() const { return m_SoftBodyInterface; }

    private:
        void InitializeJolt();
        void ShutdownJolt();

        JPH::PhysicsSystem* m_PhysicsSystem = nullptr;
        JPH::JobSystemThreadPool* m_JobSystem = nullptr;
        JPH::TempAllocatorImpl* m_TempAllocator = nullptr;
        JPH::ContactListener* m_ContactListener = nullptr;
        JPH::SoftBodyInterface* m_SoftBodyInterface = nullptr;
    };

} // namespace ecs
} // namespace ge

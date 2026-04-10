#include "Physics3D_API.h"
#include "World.h"
#include "components/Rigidbody3DComponent.h"
#include "components/TransformComponent.h"
#include "components/Collider3DComponent.h"
#include "systems/Physics3DSystem.h"

namespace ge {
namespace ecs {

bool Physics3D_API::Raycast(World& world, 
                           const Math::Vec3f& origin, 
                           const Math::Vec3f& direction,
                           float maxDistance,
                           QueryMode mode,
                           std::vector<RaycastHit>& hits) {
    (void)world;
    (void)origin;
    (void)direction;
    (void)maxDistance;
    (void)mode;
    (void)hits;
    return false;
}

bool Physics3D_API::RaycastSingle(World& world,
                         const Math::Vec3f& origin,
                         const Math::Vec3f& direction,
                         float maxDistance,
                         RaycastHit& hit) {
    (void)world;
    (void)origin;
    (void)direction;
    (void)maxDistance;
    (void)hit;
    return false;
}

bool Physics3D_API::SweepBox(World& world,
                      const Math::Vec3f& start,
                      const Math::Vec3f& end,
                      const Math::Vec3f& halfExtents,
                      const Math::Quatf& rotation,
                      std::vector<SweepHit>& hits) {
    (void)world;
    (void)start;
    (void)end;
    (void)halfExtents;
    (void)rotation;
    (void)hits;
    return false;
}

bool Physics3D_API::SweepSphere(World& world,
                         const Math::Vec3f& start,
                         const Math::Vec3f& end,
                         float radius,
                         std::vector<SweepHit>& hits) {
    (void)world;
    (void)start;
    (void)end;
    (void)radius;
    (void)hits;
    return false;
}

bool Physics3D_API::SweepCapsule(World& world,
                         const Math::Vec3f& start,
                         const Math::Vec3f& end,
                         float radius,
                         float halfHeight,
                         std::vector<SweepHit>& hits) {
    (void)world;
    (void)start;
    (void)end;
    (void)radius;
    (void)halfHeight;
    (void)hits;
    return false;
}

bool Physics3D_API::OverlapSphere(World& world,
                         const Math::Vec3f& position,
                         float radius,
                         std::vector<Entity>& overlaps) {
    (void)world;
    (void)position;
    (void)radius;
    (void)overlaps;
    return false;
}

bool Physics3D_API::OverlapBox(World& world,
                      const Math::Vec3f& position,
                      const Math::Vec3f& halfExtents,
                      const Math::Quatf& rotation,
                      std::vector<Entity>& overlaps) {
    (void)world;
    (void)position;
    (void)halfExtents;
    (void)rotation;
    (void)overlaps;
    return false;
}

bool Physics3D_API::ShapeCast(World& world,
                     const Math::Vec3f& start,
                     const Math::Vec3f& direction,
                     float maxDistance,
                     const Math::Vec3f& halfExtents,
                     const Math::Quatf& rotation,
                     std::vector<ShapeCastHit>& hits) {
    (void)world;
    (void)start;
    (void)direction;
    (void)maxDistance;
    (void)halfExtents;
    (void)rotation;
    (void)hits;
    return false;
}

bool Physics3D_API::GetRestitution(World& world, Entity entity, float& outRestitution) {
    if (!world.HasComponent<Rigidbody3DComponent>(entity)) return false;
    outRestitution = world.GetComponent<Rigidbody3DComponent>(entity).Restitution;
    return true;
}

void Physics3D_API::SetRestitution(World& world, Entity entity, float restitution) {
    if (!world.HasComponent<Rigidbody3DComponent>(entity)) return;
    world.GetComponent<Rigidbody3DComponent>(entity).Restitution = restitution;
}

bool Physics3D_API::GetFriction(World& world, Entity entity, float& outFriction) {
    if (!world.HasComponent<Rigidbody3DComponent>(entity)) return false;
    outFriction = world.GetComponent<Rigidbody3DComponent>(entity).Friction;
    return true;
}

void Physics3D_API::SetFriction(World& world, Entity entity, float friction) {
    if (!world.HasComponent<Rigidbody3DComponent>(entity)) return;
    world.GetComponent<Rigidbody3DComponent>(entity).Friction = friction;
}

bool Physics3D_API::IsCCDEnabled(World& world, Entity entity) {
    if (!world.HasComponent<Rigidbody3DComponent>(entity)) return false;
    return world.GetComponent<Rigidbody3DComponent>(entity).CCD;
}

void Physics3D_API::SetCCDEnabled(World& world, Entity entity, bool enabled) {
    if (!world.HasComponent<Rigidbody3DComponent>(entity)) return;
    world.GetComponent<Rigidbody3DComponent>(entity).CCD = enabled;
}

Math::Vec3f Physics3D_API::GetLinearVelocity(World& world, Entity entity) {
    if (!world.HasComponent<VelocityComponent>(entity)) return Math::Vec3f(0.0f, 0.0f, 0.0f);
    return world.GetComponent<VelocityComponent>(entity).Linear;
}

void Physics3D_API::SetLinearVelocity(World& world, Entity entity, const Math::Vec3f& velocity) {
    if (!world.HasComponent<Rigidbody3DComponent>(entity)) return;
    if (!world.HasComponent<VelocityComponent>(entity)) {
        world.AddComponent<VelocityComponent>(entity, VelocityComponent{});
    }
    world.GetComponent<VelocityComponent>(entity).Linear = velocity;
}

Math::Vec3f Physics3D_API::GetAngularVelocity(World& world, Entity entity) {
    if (!world.HasComponent<VelocityComponent>(entity)) return Math::Vec3f(0.0f, 0.0f, 0.0f);
    return world.GetComponent<VelocityComponent>(entity).Angular;
}

void Physics3D_API::SetAngularVelocity(World& world, Entity entity, const Math::Vec3f& velocity) {
    if (!world.HasComponent<Rigidbody3DComponent>(entity)) return;
    if (!world.HasComponent<VelocityComponent>(entity)) {
        world.AddComponent<VelocityComponent>(entity, VelocityComponent{});
    }
    world.GetComponent<VelocityComponent>(entity).Angular = velocity;
}

void Physics3D_API::AddForce(World& world, Entity entity, const Math::Vec3f& force) {
    if (!world.HasComponent<Rigidbody3DComponent>(entity)) return;
    if (!world.HasComponent<VelocityComponent>(entity)) {
        world.AddComponent<VelocityComponent>(entity, VelocityComponent{});
    }
    auto& vel = world.GetComponent<VelocityComponent>(entity);
    vel.Linear += force;
}

void Physics3D_API::AddImpulse(World& world, Entity entity, const Math::Vec3f& impulse) {
    AddForce(world, entity, impulse);
}

void Physics3D_API::AddTorque(World& world, Entity entity, const Math::Vec3f& torque) {
    (void)world;
    (void)entity;
    (void)torque;
}

void Physics3D_API::AddTorqueImpulse(World& world, Entity entity, const Math::Vec3f& impulse) {
    AddTorque(world, entity, impulse);
}

bool Physics3D_API::IsGrounded(World& world, Entity entity) {
    (void)world;
    (void)entity;
    return false;
}

void Physics3D_API::SetCollisionLayer(World& world, Entity entity, int layer) {
    if (!world.HasComponent<Rigidbody3DComponent>(entity)) return;
    world.GetComponent<Rigidbody3DComponent>(entity).CollisionLayer = layer;
}

int Physics3D_API::GetCollisionLayer(World& world, Entity entity) {
    if (!world.HasComponent<Rigidbody3DComponent>(entity)) return 0;
    return world.GetComponent<Rigidbody3DComponent>(entity).CollisionLayer;
}

void Physics3D_API::SetCollisionMask(World& world, Entity entity, int mask) {
    if (!world.HasComponent<Rigidbody3DComponent>(entity)) return;
    world.GetComponent<Rigidbody3DComponent>(entity).CollisionMask = mask;
}

int Physics3D_API::GetCollisionMask(World& world, Entity entity) {
    if (!world.HasComponent<Rigidbody3DComponent>(entity)) return 0;
    return world.GetComponent<Rigidbody3DComponent>(entity).CollisionMask;
}

void Physics3D_ScriptBindings::RegisterBindings() {
}

} // namespace ecs
} // namespace ge
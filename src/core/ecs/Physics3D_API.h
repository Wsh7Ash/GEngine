#pragma once

#include "../math/VecTypes.h"
#include "../math/quaternion.h"
#include <vector>
#include <functional>
#include <optional>

namespace ge {
namespace ecs {

class World;

struct RaycastHit {
    Entity HitEntity = INVALID_ENTITY;
    Math::Vec3f Position = {0.0f, 0.0f, 0.0f};
    Math::Vec3f Normal = {0.0f, 0.0f, 0.0f};
    float Distance = 0.0f;
    float Fraction = 1.0f;
    int SubShapeID = 0;
};

struct SweepHit {
    Entity HitEntity = INVALID_ENTITY;
    Math::Vec3f Position = {0.0f, 0.0f, 0.0f};
    Math::Vec3f Normal = {0.0f, 0.0f, 0.0f};
    float TimeOfImpact = 0.0f;
};

struct ShapeCastHit {
    Entity HitEntity = INVALID_ENTITY;
    Math::Vec3f Position = {0.0f, 0.0f, 0.0f};
    Math::Vec3f Normal = {0.0f, 0.0f, 0.0f};
    float TimeOfImpact = 0.0f;
    Math::Vec3f CollectedPoint = {0.0f, 0.0f, 0.0f};
};

enum class QueryMode {
    Closest,
    All,
    Any
};

class Physics3D_API {
public:
    static bool Raycast(World& world, 
                       const Math::Vec3f& origin, 
                       const Math::Vec3f& direction,
                       float maxDistance,
                       QueryMode mode,
                       std::vector<RaycastHit>& hits);

    static bool RaycastSingle(World& world,
                             const Math::Vec3f& origin,
                             const Math::Vec3f& direction,
                             float maxDistance,
                             RaycastHit& hit);

    static bool SweepBox(World& world,
                        const Math::Vec3f& start,
                        const Math::Vec3f& end,
                        const Math::Vec3f& halfExtents,
                        const Math::Quatf& rotation,
                        std::vector<SweepHit>& hits);

    static bool SweepSphere(World& world,
                           const Math::Vec3f& start,
                           const Math::Vec3f& end,
                           float radius,
                           std::vector<SweepHit>& hits);

    static bool SweepCapsule(World& world,
                            const Math::Vec3f& start,
                            const Math::Vec3f& end,
                            float radius,
                            float halfHeight,
                            std::vector<SweepHit>& hits);

    static bool OverlapSphere(World& world,
                             const Math::Vec3f& position,
                             float radius,
                             std::vector<Entity>& overlaps);

    static bool OverlapBox(World& world,
                          const Math::Vec3f& position,
                          const Math::Vec3f& halfExtents,
                          const Math::Quatf& rotation,
                          std::vector<Entity>& overlaps);

    static bool ShapeCast(World& world,
                         const Math::Vec3f& start,
                         const Math::Vec3f& direction,
                         float maxDistance,
                         const Math::Vec3f& halfExtents,
                         const Math::Quatf& rotation,
                         std::vector<ShapeCastHit>& hits);

    static bool GetRestitution(World& world, Entity entity, float& outRestitution);
    static void SetRestitution(World& world, Entity entity, float restitution);

    static bool GetFriction(World& world, Entity entity, float& outFriction);
    static void SetFriction(World& world, Entity entity, float friction);

    static bool IsCCDEnabled(World& world, Entity entity);
    static void SetCCDEnabled(World& world, Entity entity, bool enabled);

    static Math::Vec3f GetLinearVelocity(World& world, Entity entity);
    static void SetLinearVelocity(World& world, Entity entity, const Math::Vec3f& velocity);

    static Math::Vec3f GetAngularVelocity(World& world, Entity entity);
    static void SetAngularVelocity(World& world, Entity entity, const Math::Vec3f& velocity);

    static void AddForce(World& world, Entity entity, const Math::Vec3f& force);
    static void AddImpulse(World& world, Entity entity, const Math::Vec3f& impulse);

    static void AddTorque(World& world, Entity entity, const Math::Vec3f& torque);
    static void AddTorqueImpulse(World& world, Entity entity, const Math::Vec3f& impulse);

    static bool IsGrounded(World& world, Entity entity);

    static void SetCollisionLayer(World& world, Entity entity, int layer);
    static int GetCollisionLayer(World& world, Entity entity);

    static void SetCollisionMask(World& world, Entity entity, int mask);
    static int GetCollisionMask(World& world, Entity entity);
};

class Physics3D_ScriptBindings {
public:
    static void RegisterBindings();
};

} // namespace ecs
} // namespace ge

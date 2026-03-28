#pragma once

#include "../System.h"
#include "../../math/VecTypes.h"

namespace ge {
namespace ecs {

class DestructibleSystem : public System {
public:
    DestructibleSystem();
    ~DestructibleSystem() = default;

    void Update(World& world, float dt);

private:
    void FractureObject(World& world, Entity entity, const Math::Vec3f& impactPoint, float impactForce);
    void CreateFragments(World& world, Entity original, const Math::Vec3f& impactPoint);
};

} // namespace ecs
} // namespace ge

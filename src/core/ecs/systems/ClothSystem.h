#pragma once

#include "../System.h"
#include "../../math/VecTypes.h"

namespace ge {
namespace ecs {

class ClothSystem : public System {
public:
    ClothSystem();
    ~ClothSystem() = default;

    void Update(World& world, float dt);

private:
    void InitializeCloth(World& world, Entity entity);
    void SimulateCloth(ClothComponent& cloth, float dt, const Math::Vec3f& gravity);
    void SolveConstraints(ClothComponent& cloth, int iterations);
    void UpdateNormals(ClothComponent& cloth, uint32_t width, uint32_t height);
    void ApplyWind(ClothComponent& cloth, float dt, float time);
};

} // namespace ecs
} // namespace ge

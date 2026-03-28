#pragma once

#include "../System.h"
#include "../../math/VecTypes.h"
#include "components/TerrainComponent.h"

namespace ge {
namespace ecs {

class TerrainSystem : public System {
public:
    TerrainSystem();
    ~TerrainSystem() = default;

    void Update(World& world, float dt);

private:
    void LoadHeightmap(World& world, Entity entity, const std::string& path);
    void GeneratePhysics(World& world, Entity entity, const TerrainPhysicsSettings& settings);
    void CreateChunk(World& world, Entity terrainEntity, uint32_t chunkX, uint32_t chunkZ);
    float GetHeightAt(const TerrainComponent& terrain, float x, float z) const;
    Math::AABB CalculateBounds(const TerrainComponent& terrain) const;
};

} // namespace ecs
} // namespace ge

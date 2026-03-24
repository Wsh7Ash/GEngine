#pragma once

#include "../ecs/World.h"
#include <string>
#include <nlohmann/json.hpp>

namespace ge {
namespace scene {

/**
 * @brief Responsible for serializing and instantiating entity templates (Prefabs).
 */
class PrefabSerializer {
public:
    /**
     * @brief Serializes an entity and its entire hierarchy to a .prefab file.
     */
    static bool Serialize(ecs::World& world, ecs::Entity root, const std::string& filepath);

    /**
     * @brief Instantiates a prefab into the world.
     * @return The root entity of the newly created hierarchy.
     */
    static ecs::Entity Instantiate(ecs::World& world, const std::string& filepath);

private:
    static nlohmann::json SerializeEntity(ecs::World& world, ecs::Entity entity);
    static ecs::Entity DeserializeEntity(ecs::World& world, const nlohmann::json& data, ecs::Entity parent = ecs::INVALID_ENTITY);
};

} // namespace scene
} // namespace ge

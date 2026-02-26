#pragma once

#include "../ecs/World.h"
#include <string>

namespace ge {
namespace scene {

    /**
     * @brief Responsible for saving and loading engine scenes (ECS World states).
     */
    class SceneSerializer
    {
    public:
        SceneSerializer(ecs::World& world);

        void Serialize(const std::string& filepath);
        bool Deserialize(const std::string& filepath);

    private:
        ecs::World& world_;
    };

} // namespace scene
} // namespace ge

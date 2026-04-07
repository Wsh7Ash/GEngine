#pragma once

// ================================================================
//  SceneSerializer.h
//  ECS scene serialization with versioning and checksums.
// ================================================================

#include "../ecs/World.h"
#include "../savegame/SaveGameHeader.h"
#include "../savegame/SaveGameChecksum.h"
#include <string>

namespace ge {
namespace scene {

class SceneSerializer {
public:
    SceneSerializer(ecs::World &world);

    bool Serialize(const std::string &filepath);
    bool Deserialize(const std::string &filepath);

    bool SerializeWithHeader(const std::string& filepath, const savegame::SaveGameHeader& header);
    bool DeserializeWithHeader(const std::string& filepath, savegame::SaveGameHeader& header);
    
    std::string SerializeToString();
    bool DeserializeFromString(const std::string& data);

    static uint32_t GetVersion() { return savegame::SAVEGAME_CURRENT_VERSION; }

private:
    ecs::World &world_;
};

} // namespace scene
} // namespace ge

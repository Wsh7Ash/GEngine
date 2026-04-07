#pragma once

// ================================================================
//  SaveGameManager.h
//  Save game management with versioning, checksums, and migration.
// ================================================================

#include "../scene/SceneSerializer.h"
#include "SaveGameChecksum.h"
#include <string>
#include <vector>
#include <memory>
#include <ctime>

namespace ge {
namespace ecs {
    class World;
}
}

namespace ge {
namespace savegame {

struct SaveData {
    std::string sceneData;
    std::string playerData;
    std::string metadata;
    SaveGameHeader header;
};

class SaveGameManager {
public:
    SaveGameManager(ecs::World& world);

    bool Save(int slot, const std::string& saveName, const std::string& playerData = "");
    bool Load(int slot);
    bool Delete(int slot);
    
    SaveSlotInfo GetSlotInfo(int slot) const;
    std::vector<SaveSlotInfo> GetAllSlots() const;
    
    bool HasSave(int slot) const;
    void SetSaveDirectory(const std::string& dir);
    std::string GetSaveDirectory() const;

private:
    bool WriteSaveFile(int slot, const SaveData& data);
    bool ReadSaveFile(int slot, SaveData& data);
    bool ValidateAndMigrate(SaveData& data);
    
    std::string GetSlotPath(int slot) const;
    std::string GenerateSaveName() const;

    ecs::World& world_;
    std::string saveDirectory_;
};

} // namespace savegame
} // namespace ge
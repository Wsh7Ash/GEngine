#include "../savegame/SaveGameManager.h"
#include "../debug/log.h"
#include <fstream>
#include <sstream>
#include <filesystem>

#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#endif

namespace ge {
namespace savegame {

SaveGameManager::SaveGameManager(ecs::World& world)
    : world_(world) {
#if defined(_WIN32)
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, path))) {
        saveDirectory_ = std::string(path) + "\\GEngine\\Saves";
    }
#else
    saveDirectory_ = std::string(getenv("HOME")) + "/.geengine/saves";
#endif
    
    std::filesystem::create_directories(saveDirectory_);
}

bool SaveGameManager::Save(int slot, const std::string& saveName, const std::string& playerData) {
    if (slot < 0 || slot >= MAX_SAVE_SLOTS) {
        GE_LOG_ERROR("Invalid save slot: {}", slot);
        return false;
    }

    SaveData data;
    data.header.magic = SAVEGAME_MAGIC;
    data.header.version = SAVEGAME_CURRENT_VERSION;
    data.header.timestamp = static_cast<uint64_t>(std::time(nullptr));
    data.header.saveName = saveName;
    data.header.sceneName = "MainScene";
    data.header.engineVersion = "1.0.0";

    scene::SceneSerializer serializer(world_);
    std::stringstream sceneStream;
    
    if (!serializer.Serialize("")) {
        GE_LOG_ERROR("Failed to serialize scene");
        return false;
    }

    std::ofstream out(GetSlotPath(slot), std::ios::binary);
    if (!out.is_open()) {
        GE_LOG_ERROR("Failed to open save file for writing");
        return false;
    }

    std::string jsonStr = sceneStream.str();
    data.header.sceneDataSize = static_cast<uint32_t>(jsonStr.size());
    data.header.playerDataSize = static_cast<uint32_t>(playerData.size());
    data.header.metadataSize = 0;
    data.header.payloadSize = data.header.sceneDataSize + data.header.playerDataSize + data.header.metadataSize;

    std::string payload = jsonStr + playerData;
    data.header.checksum = SaveGameChecksum::Calculate(payload);

    out.write(reinterpret_cast<const char*>(&data.header), sizeof(SaveGameHeader));
    out.write(jsonStr.c_str(), jsonStr.size());
    out.write(playerData.c_str(), playerData.size());

    out.close();
    
    GE_LOG_INFO("Saved game to slot {}: {}", slot, saveName);
    return true;
}

bool SaveGameManager::Load(int slot) {
    if (slot < 0 || slot >= MAX_SAVE_SLOTS) {
        GE_LOG_ERROR("Invalid save slot: {}", slot);
        return false;
    }

    SaveData data;
    if (!ReadSaveFile(slot, data)) {
        return false;
    }

    if (!ValidateAndMigrate(data)) {
        GE_LOG_ERROR("Save validation failed for slot {}", slot);
        return false;
    }

    std::stringstream stream(data.sceneData);
    scene::SceneSerializer serializer(world_);
    
    GE_LOG_INFO("Loaded game from slot {}: {}", slot, data.header.saveName);
    return true;
}

bool SaveGameManager::Delete(int slot) {
    if (slot < 0 || slot >= MAX_SAVE_SLOTS) {
        return false;
    }
    
    std::string path = GetSlotPath(slot);
    if (std::filesystem::exists(path)) {
        std::filesystem::remove(path);
        GE_LOG_INFO("Deleted save slot {}", slot);
        return true;
    }
    return false;
}

SaveSlotInfo SaveGameManager::GetSlotInfo(int slot) const {
    SaveSlotInfo info;
    info.slotIndex = slot;
    
    std::ifstream in(GetSlotPath(slot), std::ios::binary);
    if (!in.is_open()) {
        info.exists = false;
        return info;
    }

    in.read(reinterpret_cast<char*>(&info.metadata), sizeof(SaveGameHeader));
    info.exists = info.metadata.IsValid();
    
    return info;
}

std::vector<SaveSlotInfo> SaveGameManager::GetAllSlots() const {
    std::vector<SaveSlotInfo> slots;
    for (int i = 0; i < MAX_SAVE_SLOTS; ++i) {
        slots.push_back(GetSlotInfo(i));
    }
    return slots;
}

bool SaveGameManager::HasSave(int slot) const {
    return std::filesystem::exists(GetSlotPath(slot));
}

void SaveGameManager::SetSaveDirectory(const std::string& dir) {
    saveDirectory_ = dir;
    std::filesystem::create_directories(saveDirectory_);
}

std::string SaveGameManager::GetSaveDirectory() const {
    return saveDirectory_;
}

bool SaveGameManager::WriteSaveFile(int slot, const SaveData& data) {
    std::ofstream out(GetSlotPath(slot), std::ios::binary);
    if (!out.is_open()) return false;
    
    out.write(reinterpret_cast<const char*>(&data.header), sizeof(SaveGameHeader));
    out.write(data.sceneData.c_str(), data.sceneData.size());
    out.write(data.playerData.c_str(), data.playerData.size());
    
    return true;
}

bool SaveGameManager::ReadSaveFile(int slot, SaveData& data) {
    std::ifstream in(GetSlotPath(slot), std::ios::binary);
    if (!in.is_open()) return false;
    
    in.read(reinterpret_cast<char*>(&data.header), sizeof(SaveGameHeader));
    
    if (!data.header.IsCompatible()) {
        GE_LOG_ERROR("Save file version mismatch");
        return false;
    }
    
    data.sceneData.resize(data.header.sceneDataSize);
    in.read(&data.sceneData[0], data.header.sceneDataSize);
    
    data.playerData.resize(data.header.playerDataSize);
    in.read(&data.playerData[0], data.header.playerDataSize);
    
    return true;
}

bool SaveGameManager::ValidateAndMigrate(SaveData& data) {
    if (data.header.magic != SAVEGAME_MAGIC) {
        GE_LOG_ERROR("Invalid save file magic");
        return false;
    }

    std::string payload = data.sceneData + data.playerData;
    uint32_t computedChecksum = SaveGameChecksum::Calculate(payload);
    
    if (computedChecksum != data.header.checksum) {
        GE_LOG_ERROR("Save file checksum mismatch! Expected {}, got {}", 
                     data.header.checksum, computedChecksum);
        return false;
    }

    if (data.header.version < SAVEGAME_CURRENT_VERSION) {
        GE_LOG_INFO("Migrating save from version {} to {}", 
                    data.header.version, SAVEGAME_CURRENT_VERSION);
    }

    return true;
}

std::string SaveGameManager::GetSlotPath(int slot) const {
    return saveDirectory_ + "/save_" + std::to_string(slot) + ".gesav";
}

std::string SaveGameManager::GenerateSaveName() const {
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);
    
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &timeinfo);
    return std::string(buffer);
}

} // namespace savegame
} // namespace ge
#pragma once

// ================================================================
//  SaveGameHeader.h
//  Save game file header with versioning and checksum.
// ================================================================

#include <cstdint>
#include <string>
#include <ctime>

namespace ge {
namespace savegame {

constexpr uint32_t SAVEGAME_MAGIC = 0x47455356;
constexpr uint32_t SAVEGAME_CURRENT_VERSION = 1;
constexpr uint32_t MAX_SAVE_SLOTS = 10;

struct SaveGameHeader {
    uint32_t magic = SAVEGAME_MAGIC;
    uint32_t version = SAVEGAME_CURRENT_VERSION;
    uint64_t timestamp = 0;
    uint32_t checksum = 0;
    uint32_t payloadSize = 0;
    uint32_t sceneDataOffset = 0;
    uint32_t sceneDataSize = 0;
    uint32_t playerDataOffset = 0;
    uint32_t playerDataSize = 0;
    uint32_t metadataOffset = 0;
    uint32_t metadataSize = 0;

    std::string sceneName;
    std::string saveName;
    std::string engineVersion;

    bool IsValid() const {
        return magic == SAVEGAME_MAGIC && 
               version <= SAVEGAME_CURRENT_VERSION &&
               payloadSize > 0;
    }

    bool IsCompatible() const {
        return magic == SAVEGAME_MAGIC && 
               version > 0 && 
               version <= SAVEGAME_CURRENT_VERSION;
    }
};

struct SaveMetadata {
    std::string saveName;
    std::string sceneName;
    std::string engineVersion;
    uint64_t timestamp = 0;
    uint32_t playTimeSeconds = 0;
    uint32_t checksum = 0;
    uint32_t size = 0;
};

struct SaveSlotInfo {
    int slotIndex = -1;
    SaveMetadata metadata;
    bool exists = false;
};

} // namespace savegame
} // namespace ge
#pragma once

// ================================================================
//  SaveGameChecksum.h
//  CRC32 checksum calculation for save game validation.
// ================================================================

#include <cstdint>
#include <vector>
#include <string>

namespace ge {
namespace savegame {

class SaveGameChecksum {
public:
    static uint32_t Calculate(const void* data, size_t length);
    static uint32_t Calculate(const std::vector<uint8_t>& data);
    static uint32_t Calculate(const std::string& str);

    static bool Verify(const void* data, size_t length, uint32_t expected);
    static bool Verify(const std::vector<uint8_t>& data, uint32_t expected);

private:
    static uint32_t crc32_table_[256];
    static bool tableInitialized_;
    static void InitTable();
};

inline uint32_t SaveGameChecksum::Calculate(const void* data, size_t length) {
    if (!tableInitialized_) {
        InitTable();
    }

    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; ++i) {
        uint8_t index = static_cast<uint8_t>((crc ^ bytes[i]) & 0xFF);
        crc = (crc >> 8) ^ crc32_table_[index];
    }

    return crc ^ 0xFFFFFFFF;
}

inline uint32_t SaveGameChecksum::Calculate(const std::vector<uint8_t>& data) {
    return Calculate(data.data(), data.size());
}

inline uint32_t SaveGameChecksum::Calculate(const std::string& str) {
    return Calculate(str.data(), str.size());
}

inline bool SaveGameChecksum::Verify(const void* data, size_t length, uint32_t expected) {
    return Calculate(data, length) == expected;
}

inline bool SaveGameChecksum::Verify(const std::vector<uint8_t>& data, uint32_t expected) {
    return Calculate(data) == expected;
}

} // namespace savegame
} // namespace ge
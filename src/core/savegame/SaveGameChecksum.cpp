#include "../savegame/SaveGameChecksum.h"

namespace ge {
namespace savegame {

uint32_t SaveGameChecksum::crc32_table_[256] = {0};
bool SaveGameChecksum::tableInitialized_ = false;

void SaveGameChecksum::InitTable() {
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (int j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
        crc32_table_[i] = crc;
    }
    tableInitialized_ = true;
}

} // namespace savegame
} // namespace ge
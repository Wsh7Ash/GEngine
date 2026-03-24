#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace ge {
namespace core {

/**
 * @brief Virtual File System for abstracting file access.
 */
class VFS {
public:
    static void Init();
    
    /**
     * @brief Reads a file as binary data.
     */
    static std::vector<uint8_t> ReadBinary(const std::string& path);

    /**
     * @brief Reads a file as a string.
     */
    static std::string ReadString(const std::string& path);

    /**
     * @brief Checks if a file exists in the virtual space.
     */
    static bool Exists(const std::string& path);

    /**
     * @brief Resolves a virtual path to a physical path (if applicable).
     */
    static std::string Resolve(const std::string& path);
};

} // namespace core
} // namespace ge

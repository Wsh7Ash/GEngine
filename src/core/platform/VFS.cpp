#include "VFS.h"
#include "../debug/log.h"
#include <fstream>
#include <filesystem>
#include <sstream>

namespace ge {
namespace core {

void VFS::Init() {
    // Initialization logic if needed (e.g., mounting PAKs)
}

std::vector<uint8_t> VFS::ReadBinary(const std::string& path) {
    std::string resolvedPath = Resolve(path);
    std::ifstream stream(resolvedPath, std::ios::binary | std::ios::ate);

    if (!stream) {
        GE_LOG_ERROR("VFS: Failed to open binary file: %s", path.c_str());
        return {};
    }

    std::streamsize size = stream.tellg();
    stream.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!stream.read(reinterpret_cast<char*>(buffer.data()), size)) {
        GE_LOG_ERROR("VFS: Failed to read binary file: %s", path.c_str());
        return {};
    }

    return buffer;
}

std::string VFS::ReadString(const std::string& path) {
    std::string resolvedPath = Resolve(path);
    std::ifstream stream(resolvedPath);

    if (!stream) {
        GE_LOG_ERROR("VFS: Failed to open string file: %s", path.c_str());
        return "";
    }

    std::stringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

bool VFS::Exists(const std::string& path) {
    return std::filesystem::exists(Resolve(path));
}

std::string VFS::Resolve(const std::string& path) {
    // For now, simple path resolution
    // In the future, this would check mounted pak files first
    return path;
}

} // namespace core
} // namespace ge

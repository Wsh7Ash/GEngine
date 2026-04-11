#include "VFS.h"
#include "../debug/log.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace ge {
namespace core {

namespace {

std::filesystem::path NormalizePath(const std::filesystem::path& path) {
    std::error_code ec;

    const auto weaklyCanonical = std::filesystem::weakly_canonical(path, ec);
    if (!ec) {
        return weaklyCanonical;
    }

    const auto absolute = std::filesystem::absolute(path, ec);
    if (!ec) {
        return absolute.lexically_normal();
    }

    return path.lexically_normal();
}

std::filesystem::path ResolveProjectRootPath() {
    std::error_code ec;
    std::filesystem::path current = std::filesystem::current_path(ec);
    if (ec || current.empty()) {
        current = ".";
    }

    for (int depth = 0; depth < 10; ++depth) {
        if (std::filesystem::exists(current / "CMakeLists.txt", ec) &&
            std::filesystem::exists(current / "src", ec)) {
            return NormalizePath(current);
        }

        if (!current.has_parent_path()) {
            break;
        }

        const auto parent = current.parent_path();
        if (parent == current) {
            break;
        }

        current = parent;
    }

    return {};
}

std::filesystem::path ResolveCandidatePath(const std::filesystem::path& candidate) {
    std::error_code ec;
    if (candidate.empty()) {
        return {};
    }

    if (std::filesystem::exists(candidate, ec)) {
        return NormalizePath(candidate);
    }

    return {};
}

} // namespace

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
    if (path.empty()) {
        return path;
    }

    const std::filesystem::path inputPath(path);

    if (inputPath.is_absolute()) {
        return NormalizePath(inputPath).string();
    }

    if (const auto directCandidate = ResolveCandidatePath(inputPath); !directCandidate.empty()) {
        return directCandidate.string();
    }

    const std::filesystem::path projectRoot = ResolveProjectRootPath();
    if (!projectRoot.empty()) {
        if (const auto projectCandidate = ResolveCandidatePath(projectRoot / inputPath);
            !projectCandidate.empty()) {
            return projectCandidate.string();
        }
    }

    return inputPath.lexically_normal().string();
}

} // namespace core
} // namespace ge

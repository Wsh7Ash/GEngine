#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace ge {
namespace editor {

/**
 * @brief Utility class for integrating with Visual Studio Code.
 */
class VSCodeUtility {
public:
    /**
     * @brief Attempts to find the path to the VS Code executable.
     * @return Absolute path to code.exe, or empty string if not found.
     */
    static std::string FindVSCodeExecutable();

    /**
     * @brief Opens a file or folder in VS Code.
     * @param path The absolute path to open.
     */
    static void OpenInVSCode(const std::string& path);

    /**
     * @brief Generates the .vscode configuration files in the project root.
     * @param projectRoot The root directory of the project.
     * @param includePaths List of include directories to add to c_cpp_properties.json.
     */
    static void GenerateVSCodeConfig(const std::filesystem::path& projectRoot, const std::vector<std::string>& includePaths);

private:
    static bool WriteFile(const std::filesystem::path& path, const std::string& content);
};

} // namespace editor
} // namespace ge

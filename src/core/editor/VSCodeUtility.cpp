#include "VSCodeUtility.h"
#include "../debug/log.h"
#include <fstream>
#include <cstdlib>
#include <filesystem>

#ifdef GE_PLATFORM_WINDOWS
#include <windows.h>
#include <shellapi.h>
#endif

namespace ge {
namespace editor {

std::string VSCodeUtility::FindVSCodeExecutable() {
    // 1. Check PATH
#ifdef GE_PLATFORM_WINDOWS
    char* pathEnv = nullptr;
    size_t len = 0;
    if (_dupenv_s(&pathEnv, &len, "PATH") == 0 && pathEnv != nullptr) {
        std::string paths = pathEnv;
        free(pathEnv);
        // This is a bit complex to parse manually, so we'll also check common locations
    }

    // 2. Check common locations
    std::vector<std::string> commonPaths;
    const char* localAppData = std::getenv("LOCALAPPDATA");
    if (localAppData) {
        commonPaths.push_back(std::string(localAppData) + "/Programs/Microsoft VS Code/bin/code.cmd");
    }
    commonPaths.push_back("C:/Program Files/Microsoft VS Code/bin/code.cmd");
    commonPaths.push_back("C:/Program Files (x86)/Microsoft VS Code/bin/code.cmd");

    for (const auto& path : commonPaths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
#else
    // Linux/macOS
    const char* pathEnv = std::getenv("PATH");
    if (pathEnv) {
        std::string paths = pathEnv;
        std::string::size_type start = 0;
        std::string::size_type end;
        while ((end = paths.find(':', start)) != std::string::npos) {
            std::string dir = paths.substr(start, end - start);
            std::filesystem::path codePath = std::filesystem::path(dir) / "code";
            if (std::filesystem::exists(codePath)) {
                return codePath.string();
            }
            start = end + 1;
        }
        std::string dir = paths.substr(start);
        if (!dir.empty()) {
            std::filesystem::path codePath = std::filesystem::path(dir) / "code";
            if (std::filesystem::exists(codePath)) {
                return codePath.string();
            }
        }
    }

    // Check common install locations
    std::vector<std::string> commonPaths;
    commonPaths.push_back("/usr/bin/code");
    commonPaths.push_back("/usr/local/bin/code");
    commonPaths.push_back("/opt/vscode/bin/code");
    commonPaths.push_back("~/Applications/Visual Studio Code.app/Contents/MacOS/Electron");

    for (const auto& path : commonPaths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
#endif

    return "";
}

void VSCodeUtility::OpenInVSCode(const std::string& path) {
    std::string codePath = FindVSCodeExecutable();
    if (codePath.empty()) {
        GE_LOG_ERROR("VS Code not found in PATH or common locations.");
        return;
    }

#ifdef GE_PLATFORM_WINDOWS
    // Use code.cmd to open. Use double quotes carefully.
    std::string command = "/c \"\"" + codePath + "\" \"" + path + "\"\"";
    ShellExecuteA(NULL, "open", "cmd.exe", command.c_str(), NULL, SW_HIDE);
#else
    std::string command = "\"" + codePath + "\" \"" + path + "\" &";
    system(command.c_str());
#endif
    GE_LOG_INFO("Opening in VS Code: {0}", path.c_str());
}

void VSCodeUtility::OpenInExplorer(const std::string& path) {
#ifdef GE_PLATFORM_WINDOWS
    std::error_code ec;
    const std::filesystem::path absolute = std::filesystem::absolute(path, ec);
    const std::filesystem::path target = ec ? std::filesystem::path(path) : absolute;

    if (std::filesystem::is_directory(target, ec)) {
        ShellExecuteA(NULL, "open", target.string().c_str(), NULL, NULL, SW_SHOWNORMAL);
        return;
    }

    std::string args = "/select,\"" + target.string() + "\"";
    ShellExecuteA(NULL, "open", "explorer.exe", args.c_str(), NULL, SW_SHOWNORMAL);
#else
    std::string command = "xdg-open \"" + path + "\" &";
    system(command.c_str());
#endif
}

void VSCodeUtility::GenerateVSCodeConfig(const std::filesystem::path& projectRoot, const std::vector<std::string>& includePaths) {
    std::filesystem::path vscodeDir = projectRoot / ".vscode";
    if (!std::filesystem::exists(vscodeDir)) {
        std::filesystem::create_directories(vscodeDir);
    }

    // 1. c_cpp_properties.json
    std::string cppProps = R"cppprops({
    "configurations": [
        {
            "name": "GEngine",
            "includePath": [
                "${workspaceFolder}/**",
)cppprops";
    for (const auto& include : includePaths) {
        cppProps += "                \"" + include + "\",\n";
    }
    // Remove last comma and newline if any, then close
    if (!includePaths.empty()) {
        cppProps.pop_back(); // \n
        cppProps.pop_back(); // ,
        cppProps += "\n";
    }
    cppProps += R"cppprops(            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE",
                "GE_PLATFORM_WINDOWS"
            ],
            "windowsSdkVersion": "10.0.26100.0",
            "compilerPath": "cl.exe",
            "cStandard": "c17",
            "cppStandard": "c++20",
            "intelliSenseMode": "windows-msvc-x64"
        }
    ],
    "version": 4
})cppprops";
    WriteFile(vscodeDir / "c_cpp_properties.json", cppProps);

    // 2. tasks.json
    std::string tasks = R"tasks({
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build GEngine",
            "type": "shell",
            "command": "cmake --build build --config Debug --target GameEngine",
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": "$msCompile"
        }
    ]
})tasks";
    WriteFile(vscodeDir / "tasks.json", tasks);

    // 3. launch.json (Supports both MSVC and lldb/codelldb)
    std::string launch = R"launch({
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug (codelldb)",
            "type": "lldb",
            "request": "launch",
            "program": "${workspaceFolder}/build/bin/Debug/GameEngine.exe",
            "args": [],
            "cwd": "${workspaceFolder}/build/bin/Debug",
            "stopOnEntry": false,
            "preLaunchTask": "Build GEngine"
        },
        {
            "name": "Debug (MSVC)",
            "type": "cppvsdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/bin/Debug/GameEngine.exe",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}/build/bin/Debug",
            "environment": [],
            "console": "integratedTerminal",
            "preLaunchTask": "Build GEngine"
        }
    ]
})launch";
    WriteFile(vscodeDir / "launch.json", launch);

    GE_LOG_INFO("Generated .vscode configuration in {0}", projectRoot.string().c_str());
}

bool VSCodeUtility::WriteFile(const std::filesystem::path& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        GE_LOG_ERROR("Failed to write VS Code config file: {0}", path.string().c_str());
        return false;
    }
    file << content;
    return true;
}

} // namespace editor
} // namespace ge

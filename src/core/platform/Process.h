#pragma once

// ================================================================
//  Process.h
//  Cross-platform process management.
// ================================================================

#include "Thread.h"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace ge {
namespace platform {

enum class ProcessPriority {
    Idle,
    BelowNormal,
    Normal,
    AboveNormal,
    High,
    Realtime
};

class Process {
public:
    using ID = uint64_t;
    using ExitCode = int32_t;

    static std::unique_ptr<Process> Spawn(const std::string& command, const std::vector<std::string>& args = {});
    static std::unique_ptr<Process> Spawn(const std::string& executable, const std::vector<std::string>& args, const std::string& workingDirectory = "");
    static ID GetCurrentProcessID();
    static Process* GetCurrentProcess();

    virtual ~Process() = default;

    virtual ID GetID() const = 0;
    virtual ExitCode GetExitCode() const = 0;
    virtual bool IsRunning() const = 0;
    virtual bool HasExited() const = 0;

    virtual bool Wait(int32_t timeoutMs = -1) = 0;
    virtual bool Terminate(int32_t exitCode = -1) = 0;
    virtual bool Kill() = 0;

    virtual int32_t GetReturnCode() const = 0;

    virtual void SetPriority(ProcessPriority priority) = 0;
    virtual ProcessPriority GetPriority() const = 0;

    virtual void SetWorkingDirectory(const std::string& path) = 0;
    virtual std::string GetWorkingDirectory() const = 0;

    virtual void SetEnvironmentVariable(const std::string& name, const std::string& value) = 0;
    virtual std::string GetEnvironmentVariable(const std::string& name) const = 0;
    virtual std::vector<std::string> GetEnvironmentVariables() const = 0;

    virtual int GetStandardInput() const = 0;
    virtual int GetStandardOutput() const = 0;
    virtual int GetStandardError() const = 0;

    virtual void SetStdIn(const std::string& input) = 0;
    virtual std::string GetStdOut() const = 0;
    virtual std::string GetStdErr() const = 0;

    struct Statistics {
        uint64_t memoryUsage = 0;
        uint64_t virtualMemory = 0;
        double cpuUsage = 0.0;
        int threadCount = 0;
        int handleCount = 0;
    };

    virtual Statistics GetStatistics() const = 0;
};

class SharedLibrary {
public:
    static std::unique_ptr<SharedLibrary> Load(const std::string& path);
    static std::string GetLastError();

    virtual ~SharedLibrary() = default;

    virtual bool IsLoaded() const = 0;
    virtual void Unload() = 0;

    virtual void* GetSymbol(const std::string& name) const = 0;
    virtual bool HasSymbol(const std::string& name) const = 0;

    virtual const std::string& GetPath() const = 0;
    virtual const std::string& GetName() const = 0;

protected:
    SharedLibrary() = default;
    std::string path_;
    std::string name_;
};

class DynamicSymbol {
public:
    DynamicSymbol() = default;
    DynamicSymbol(SharedLibrary* library, const std::string& name);
    ~DynamicSymbol() = default;

    bool IsValid() const { return symbol_ != nullptr; }
    explicit operator bool() const { return IsValid(); }

    template<typename T>
    T* Get() const { return reinterpret_cast<T*>(symbol_); }

private:
    void* symbol_ = nullptr;
    SharedLibrary* library_ = nullptr;
};

class CommandLine {
public:
    static std::string Join(const std::vector<std::string>& args);
    static std::vector<std::string> Split(const std::string& commandLine);
    static std::string Quote(const std::string& arg);

    static std::string GetExecutablePath();
    static std::string GetCurrentExecutable();
};

class Environment {
public:
    static std::string Get(const std::string& name, const std::string& defaultValue = "");
    static void Set(const std::string& name, const std::string& value);
    static void Remove(const std::string& name);

    static bool Has(const std::string& name);

    static std::vector<std::string> GetAll();
    static void Clear();

    static std::string GetPath();
    static void SetPath(const std::string& path);
};

} // namespace platform
} // namespace ge

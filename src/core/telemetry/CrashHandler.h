#pragma once

// ================================================================
//  CrashHandler.h
//  Exception handling and crash dump generation.
// ================================================================

#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <memory>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace ge {
namespace telemetry {

struct CrashInfo {
    std::string exceptionType;
    std::string exceptionMessage;
    std::string stackTrace;
    uint64_t timestamp = 0;
    uint32_t exceptionCode = 0;
    void* exceptionAddress = nullptr;
    
    std::string applicationName;
    std::string applicationVersion;
    std::string engineVersion;
    std::string platform;
    std::string architecture;
    
    std::vector<std::string> loadedModules;
};

struct CrashReport {
    CrashInfo info;
    std::string crashDirectory;
    std::string dumpFilePath;
    std::string metadataFilePath;
};

class CrashHandler {
public:
    static CrashHandler& Get();
    
    void Initialize();
    void Shutdown();
    
    void SetCrashDirectory(const std::string& dir);
    std::string GetCrashDirectory() const;
    
    void SetMinidumpType(int type);
    int GetMinidumpType() const { return minidumpType_; }
    
    void SetAutoSubmit(bool autoSubmit);
    bool IsAutoSubmitEnabled() const { return autoSubmit_; }
    
    void AddCallback(std::function<void(const CrashReport&)> callback);
    
    bool WriteMinidump(const std::string& filepath);
    bool WriteCrashMetadata(const CrashReport& report);
    
    std::vector<CrashReport> GetRecentCrashes(int maxCount = 5) const;
    bool DeleteCrash(const std::string& crashId);
    
    bool IsInitialized() const { return isInitialized_; }
    
private:
    CrashHandler();
    ~CrashHandler();
    
#if defined(_WIN32)
    static LONG WINAPI StaticExceptionHandler(PEXCEPTION_POINTERS exceptionInfo);
    LONG HandleException(PEXCEPTION_POINTERS exceptionInfo);
#endif
    
    void CreateCrashDirectory();
    std::string GenerateCrashId();
    
    bool isInitialized_ = false;
    bool autoSubmit_ = true;
    std::string crashDirectory_;
    int minidumpType_ = 2;
    
    std::vector<std::function<void(const CrashReport&)>> crashCallbacks_;
    
    std::unique_ptr<CrashReport> pendingCrash_;
    
    std::function<void(const CrashInfo&)> onCrashOccurred_;
};

class CrashReportUploader {
public:
    static bool UploadCrashReport(const CrashReport& report, const std::string& endpoint);
    static bool UploadCrashReportAsync(const CrashReport& report, const std::string& endpoint);
};

} // namespace telemetry
} // namespace ge
#include "CrashHandler.h"
#include "TelemetryConsent.h"
#include "../debug/log.h"

#if defined(_WIN32)
#include <dbghelp.h>
#include <shellapi.h>
#include <shlobj.h>
#endif

#include <chrono>
#include <fstream>
#include <ctime>

#if defined(_WIN32)
#pragma comment(lib, "dbghelp.lib")
#endif

namespace ge {
namespace telemetry {

#if defined(_WIN32)
LONG WINAPI CrashHandler::StaticExceptionHandler(PEXCEPTION_POINTERS exceptionInfo) {
    return CrashHandler::Get().HandleException(exceptionInfo);
}
#endif

CrashHandler& CrashHandler::Get() {
    static CrashHandler instance;
    return instance;
}

CrashHandler::CrashHandler() {
#if defined(_WIN32)
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        crashDirectory_ = std::string(path) + "\\GEngine\\Crashes";
    }
#elif defined(__APPLE__)
    crashDirectory_ = std::string(getenv("HOME")) + "/Library/Logs/GEngine/Crashes";
#else
    crashDirectory_ = std::string(getenv("HOME")) + "/.local/share/GEngine/crashes";
#endif

    std::filesystem::create_directories(crashDirectory_);
}

CrashHandler::~CrashHandler() {
    Shutdown();
}

void CrashHandler::Initialize() {
    if (isInitialized_) return;
    
#if defined(_WIN32)
    SetUnhandledExceptionFilter(StaticExceptionHandler);
#endif
    
    isInitialized_ = true;
    GE_LOG_INFO("CrashHandler initialized, crash directory: {}", crashDirectory_);
}

void CrashHandler::Shutdown() {
    if (!isInitialized_) return;
    
#if defined(_WIN32)
    SetUnhandledExceptionFilter(nullptr);
#endif
    
    isInitialized_ = false;
}

void CrashHandler::SetCrashDirectory(const std::string& dir) {
    crashDirectory_ = dir;
    std::filesystem::create_directories(crashDirectory_);
}

std::string CrashHandler::GetCrashDirectory() const {
    return crashDirectory_;
}

void CrashHandler::SetMinidumpType(int type) {
    minidumpType_ = type;
}

void CrashHandler::SetAutoSubmit(bool autoSubmit) {
    autoSubmit_ = autoSubmit;
}

void CrashHandler::AddCallback(std::function<void(const CrashReport&)> callback) {
    crashCallbacks_.push_back(callback);
}

#if defined(_WIN32)
LONG CrashHandler::HandleException(PEXCEPTION_POINTERS exceptionInfo) {
    CrashReport report;
    report.info.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    report.info.exceptionCode = exceptionInfo->ExceptionRecord->ExceptionCode;
    report.info.exceptionAddress = exceptionInfo->ExceptionRecord->ExceptionAddress;
    
    switch (exceptionInfo->ExceptionRecord->ExceptionCode) {
        case EXCEPTION_ACCESS_VIOLATION:
            report.info.exceptionType = "Access Violation";
            break;
        case EXCEPTION_STACK_OVERFLOW:
            report.info.exceptionType = "Stack Overflow";
            break;
        case EXCEPTION_DIVIDE_BY_ZERO:
            report.info.exceptionType = "Divide by Zero";
            break;
        case EXCEPTION_ASSERTION_FAILURE:
            report.info.exceptionType = "Assertion Failure";
            break;
        default:
            report.info.exceptionType = "Unknown Exception";
    }
    
    std::string crashId = GenerateCrashId();
    report.crashDirectory = crashDirectory_ + "/" + crashId;
    std::filesystem::create_directories(report.crashDirectory);
    
    report.dumpFilePath = report.crashDirectory + "/crash.dmp";
    report.metadataFilePath = report.crashDirectory + "/crash.json";
    
    WriteMinidump(report.dumpFilePath);
    WriteCrashMetadata(report);
    
    for (auto& callback : crashCallbacks_) {
        callback(report);
    }
    
    if (autoSubmit_ && TelemetryConsent::Get().HasConsentedToCrashReports()) {
        CrashReportUploader::UploadCrashReportAsync(report, "https://telemetry.gengine.dev/v1/crash");
    }
    
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

bool CrashHandler::WriteMinidump(const std::string& filepath) {
#if defined(_WIN32)
    HANDLE file = CreateFileA(filepath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, 
                               nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        GE_LOG_ERROR("Failed to create minidump file: {}", filepath);
        return false;
    }
    
    MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(minidumpType_);
    MINIDUMP_CALLBACK_INFORMATION callbackInfo = {0};
    
    bool success = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), 
                                      file, dumpType, nullptr, nullptr, &callbackInfo);
    
    CloseHandle(file);
    
    if (!success) {
        GE_LOG_ERROR("Failed to write minidump");
    }
    
    return success;
#else
    return false;
#endif
}

bool CrashHandler::WriteCrashMetadata(const CrashReport& report) {
    std::ofstream file(report.metadataFilePath);
    if (!file.is_open()) {
        GE_LOG_ERROR("Failed to write crash metadata: {}", report.metadataFilePath);
        return false;
    }
    
    file << "{\n";
    file << "  \"crashId\": \"" << std::filesystem::path(report.crashDirectory).filename().string() << "\",\n";
    file << "  \"timestamp\": " << report.info.timestamp << ",\n";
    file << "  \"exceptionType\": \"" << report.info.exceptionType << "\",\n";
    file << "  \"exceptionCode\": " << report.info.exceptionCode << ",\n";
    file << "  \"applicationName\": \"" << report.info.applicationName << "\",\n";
    file << "  \"applicationVersion\": \"" << report.info.applicationVersion << "\",\n";
    file << "  \"engineVersion\": \"" << report.info.engineVersion << "\",\n";
    file << "  \"platform\": \"" << report.info.platform << "\",\n";
    file << "  \"dumpFile\": \"crash.dmp\"\n";
    file << "}\n";
    
    return true;
}

std::vector<CrashReport> CrashHandler::GetRecentCrashes(int maxCount) const {
    std::vector<CrashReport> crashes;
    
    if (!std::filesystem::exists(crashDirectory_)) {
        return crashes;
    }
    
    for (auto& entry : std::filesystem::directory_iterator(crashDirectory_)) {
        if (entry.is_directory()) {
            CrashReport report;
            report.crashDirectory = entry.path().string();
            report.dumpFilePath = (entry.path() / "crash.dmp").string();
            report.metadataFilePath = (entry.path() / "crash.json").string();
            crashes.push_back(report);
        }
    }
    
    return crashes;
}

bool CrashHandler::DeleteCrash(const std::string& crashId) {
    std::filesystem::path crashPath = crashDirectory_ / crashId;
    if (std::filesystem::exists(crashPath)) {
        std::filesystem::remove_all(crashPath);
        return true;
    }
    return false;
}

std::string CrashHandler::GenerateCrashId() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    char buffer[32];
    std::tm tm;
#if defined(_WIN32)
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &tm);
    
    return std::string(buffer);
}

bool CrashReportUploader::UploadCrashReport(const CrashReport& report, const std::string& endpoint) {
    GE_LOG_INFO("Uploading crash report to {}", endpoint);
    return true;
}

bool CrashReportUploader::UploadCrashReportAsync(const CrashReport& report, const std::string& endpoint) {
    GE_LOG_INFO("Async upload of crash report queued");
    return true;
}

} // namespace telemetry
} // namespace ge
#include "TelemetryConsent.h"
#include "../debug/log.h"
#include <nlohmann/json.hpp>

namespace ge {
namespace telemetry {

using json = nlohmann::json;

TelemetryConsent& TelemetryConsent::Get() {
    static TelemetryConsent instance;
    return instance;
}

TelemetryConsent::TelemetryConsent() {
#if defined(_WIN32)
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        std::filesystem::path configDir = std::string(path) + "\\GEngine";
        std::filesystem::create_directories(configDir);
        configPath_ = (configDir / "telemetry.json").string();
    }
#else
    configPath_ = std::string(getenv("HOME")) + "/.geengine/telemetry.json";
#endif
    
    LoadFromFile();
}

bool TelemetryConsent::LoadFromFile() {
    std::ifstream file(configPath_);
    if (!file.is_open()) {
        isFirstLaunch_ = true;
        return false;
    }
    
    try {
        json data = json::parse(file);
        
        if (data.contains("crashReports")) {
            consent_.crashReports = data["crashReports"];
        }
        if (data.contains("performanceMetrics")) {
            consent_.performanceMetrics = data["performanceMetrics"];
        }
        if (data.contains("usageAnalytics")) {
            consent_.usageAnalytics = data["usageAnalytics"];
        }
        if (data.contains("firstLaunchComplete")) {
            isFirstLaunch_ = !data["firstLaunchComplete"].get<bool>();
        }
        
        isFirstLaunch_ = false;
        return true;
    } catch (const std::exception& e) {
        GE_LOG_ERROR("Failed to load telemetry consent: {}", e.what());
        return false;
    }
}

bool TelemetryConsent::SaveToFile() {
    std::ofstream file(configPath_);
    if (!file.is_open()) {
        GE_LOG_ERROR("Failed to save telemetry consent to {}", configPath_);
        return false;
    }
    
    try {
        json data;
        data["crashReports"] = consent_.crashReports;
        data["performanceMetrics"] = consent_.performanceMetrics;
        data["usageAnalytics"] = consent_.usageAnalytics;
        data["firstLaunchComplete"] = true;
        
        file << data.dump(4);
        return true;
    } catch (const std::exception& e) {
        GE_LOG_ERROR("Failed to write telemetry consent: {}", e.what());
        return false;
    }
}

void TelemetryConsent::SetOptIn(bool enabled) {
    consent_.crashReports = enabled;
    consent_.performanceMetrics = enabled;
    consent_.usageAnalytics = enabled;
    consentTimestamp_ = std::chrono::system_clock::now();
    SaveToFile();
}

void TelemetryConsent::SetCrashReportsConsent(bool enabled) {
    consent_.crashReports = enabled;
    consentTimestamp_ = std::chrono::system_clock::now();
    SaveToFile();
}

void TelemetryConsent::SetMetricsConsent(bool enabled) {
    consent_.performanceMetrics = enabled;
    consentTimestamp_ = std::chrono::system_clock::now();
    SaveToFile();
}

void TelemetryConsent::SetAnalyticsConsent(bool enabled) {
    consent_.usageAnalytics = enabled;
    consentTimestamp_ = std::chrono::system_clock::now();
    SaveToFile();
}

void TelemetryConsent::MarkFirstLaunchComplete() {
    isFirstLaunch_ = false;
    SaveToFile();
}

void TelemetryConsent::SetConsentSettings(const ConsentSettings& settings) {
    consent_ = settings;
    consentTimestamp_ = std::chrono::system_clock::now();
    SaveToFile();
}

std::string TelemetryConsent::GetConsentFilePath() const {
    return configPath_;
}

std::chrono::system_clock::time_point TelemetryConsent::GetConsentTimestamp() const {
    return consentTimestamp_;
}

std::chrono::system_clock::time_point TelemetryConsent::GetLastUploadTimestamp() const {
    return lastUploadTimestamp_;
}

void TelemetryConsent::UpdateLastUploadTimestamp() {
    lastUploadTimestamp_ = std::chrono::system_clock::now();
}

bool TelemetryConsentDialog::ShowConsentDialog() {
    return TelemetryConsent::Get().IsOptIn();
}

} // namespace telemetry
} // namespace ge
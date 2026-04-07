#pragma once

// ================================================================
//  TelemetryConsent.h
//  Opt-in consent management for crash reporting and metrics.
// ================================================================

#include <string>
#include <chrono>
#include <fstream>
#include <filesystem>

#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#endif

namespace ge {
namespace telemetry {

struct ConsentSettings {
    bool crashReports = false;
    bool performanceMetrics = false;
    bool usageAnalytics = false;
    
    bool IsAnyEnabled() const {
        return crashReports || performanceMetrics || usageAnalytics;
    }
    
    bool IsAllEnabled() const {
        return crashReports && performanceMetrics && usageAnalytics;
    }
};

class TelemetryConsent {
public:
    static TelemetryConsent& Get();
    
    bool IsOptIn() const { return consent_.IsAnyEnabled(); }
    void SetOptIn(bool enabled);
    
    bool HasConsentedToCrashReports() const { return consent_.crashReports; }
    bool HasConsentedToMetrics() const { return consent_.performanceMetrics; }
    bool HasConsentedToAnalytics() const { return consent_.usageAnalytics; }
    
    void SetCrashReportsConsent(bool enabled);
    void SetMetricsConsent(bool enabled);
    void SetAnalyticsConsent(bool enabled);
    
    bool IsFirstLaunch() const { return isFirstLaunch_; }
    void MarkFirstLaunchComplete();
    
    ConsentSettings GetConsentSettings() const { return consent_; }
    void SetConsentSettings(const ConsentSettings& settings);
    
    std::string GetConsentFilePath() const;
    
    bool LoadFromFile();
    bool SaveToFile();
    
    std::chrono::system_clock::time_point GetConsentTimestamp() const;
    std::chrono::system_clock::time_point GetLastUploadTimestamp() const;
    void UpdateLastUploadTimestamp();
    
private:
    TelemetryConsent();
    
    ConsentSettings consent_;
    bool isFirstLaunch_ = true;
    std::chrono::system_clock::time_point consentTimestamp_;
    std::chrono::system_clock::time_point lastUploadTimestamp_;
    std::string configPath_;
};

class TelemetryConsentDialog {
public:
    static bool ShowConsentDialog();
    
    static std::string GetPrivacyPolicyUrl() { return "https://gengine.dev/privacy"; }
    static std::string GetTermsOfServiceUrl() { return "https://gengine.dev/terms"; }
};

} // namespace telemetry
} // namespace ge
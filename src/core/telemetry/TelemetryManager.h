#pragma once

// ================================================================
//  TelemetryManager.h
//  Coordinates crash handling, consent, and telemetry upload.
// ================================================================

#include "TelemetryConsent.h"
#include "CrashHandler.h"
#include "TelemetryUploader.h"
#include "../debug/TelemetrySession.h"
#include <memory>
#include <atomic>

namespace ge {
namespace telemetry {

class TelemetryManager {
public:
    static TelemetryManager& Get();
    
    void Initialize();
    void Shutdown();
    
    void SetEnabled(bool enabled);
    bool IsEnabled() const { return enabled_; }
    
    void SetUploadEndpoint(const std::string& endpoint);
    std::string GetUploadEndpoint() const;
    
    bool IsTelemetryAllowed() const;
    bool IsCrashReportingAllowed() const;
    
    void CaptureSession(debug::TelemetrySession* session);
    void FlushAndUpload();
    
    void SetApplicationInfo(const std::string& name, const std::string& version);
    
    struct ManagerStats {
        bool crashHandlerInitialized = false;
        bool uploaderInitialized = false;
        bool isConsentGiven = false;
        size_t pendingUploads = 0;
    };
    
    const ManagerStats& GetStats() const { return stats_; }
    
private:
    TelemetryManager();
    ~TelemetryManager();
    
    bool enabled_ = false;
    ManagerStats stats_;
    
    std::string applicationName_;
    std::string applicationVersion_;
};

class TelemetryAutoSession {
public:
    TelemetryAutoSession(const std::string& name);
    ~TelemetryAutoSession();
    
    debug::TelemetrySession* GetSession() { return session_.get(); }
    
private:
    std::unique_ptr<debug::TelemetrySession> session_;
};

} // namespace telemetry
} // namespace ge
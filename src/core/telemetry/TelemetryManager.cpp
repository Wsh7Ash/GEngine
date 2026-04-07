#include "TelemetryManager.h"
#include "../debug/log.h"

namespace ge {
namespace telemetry {

TelemetryManager& TelemetryManager::Get() {
    static TelemetryManager instance;
    return instance;
}

TelemetryManager::TelemetryManager() {
}

TelemetryManager::~TelemetryManager() {
    Shutdown();
}

void TelemetryManager::Initialize() {
    if (enabled_) return;
    
    enabled_ = true;
    
    stats_.isConsentGiven = TelemetryConsent::Get().IsOptIn();
    
    if (stats_.isConsentGiven) {
        CrashHandler::Get().Initialize();
        stats_.crashHandlerInitialized = true;
        
        if (TelemetryConsent::Get().HasConsentedToMetrics()) {
            TelemetryUploader::Get().Initialize();
            stats_.uploaderInitialized = true;
        }
    }
    
    GE_LOG_INFO("TelemetryManager initialized, consent: {}", stats_.isConsentGiven);
}

void TelemetryManager::Shutdown() {
    if (!enabled_) return;
    
    if (stats_.uploaderInitialized) {
        FlushAndUpload();
        TelemetryUploader::Get().Shutdown();
    }
    
    if (stats_.crashHandlerInitialized) {
        CrashHandler::Get().Shutdown();
    }
    
    enabled_ = false;
}

void TelemetryManager::SetEnabled(bool enabled) {
    if (enabled && !enabled_) {
        Initialize();
    } else if (!enabled && enabled_) {
        Shutdown();
    }
}

void TelemetryManager::SetUploadEndpoint(const std::string& endpoint) {
    TelemetryUploader::Get().SetEndpoint(endpoint);
}

std::string TelemetryManager::GetUploadEndpoint() const {
    return TelemetryUploader::Get().GetEndpoint();
}

bool TelemetryManager::IsTelemetryAllowed() const {
    return enabled_ && TelemetryConsent::Get().HasConsentedToMetrics();
}

bool TelemetryManager::IsCrashReportingAllowed() const {
    return enabled_ && TelemetryConsent::Get().HasConsentedToCrashReports();
}

void TelemetryManager::CaptureSession(debug::TelemetrySession* session) {
    if (!IsTelemetryAllowed() || !session) return;
    
    TelemetryUploader::Get().QueueSessionUpload(session);
    stats_.pendingUploads = TelemetryUploader::Get().GetQueueSize();
}

void TelemetryManager::FlushAndUpload() {
    TelemetryUploader::Get().SetEnabled(false);
    TelemetryUploader::Get().SetEnabled(true);
}

void TelemetryManager::SetApplicationInfo(const std::string& name, const std::string& version) {
    applicationName_ = name;
    applicationVersion_ = version;
    
    if (stats_.crashHandlerInitialized) {
        CrashInfo info;
        info.applicationName = name;
        info.applicationVersion = version;
    }
}

TelemetryAutoSession::TelemetryAutoSession(const std::string& name) {
    session_ = std::make_unique<debug::TelemetrySession>(name);
    session_->Start();
}

TelemetryAutoSession::~TelemetryAutoSession() {
    if (session_ && session_->IsActive()) {
        session_->Stop();
        
        if (TelemetryManager::Get().IsTelemetryAllowed()) {
            TelemetryManager::Get().CaptureSession(session_.get());
        }
    }
}

} // namespace telemetry
} // namespace ge
#pragma once

// ================================================================
//  TelemetrySession.h
//  Session capture with export to various formats.
// ================================================================

#include "ProfilerEx.h"
#include <string>
#include <vector>
#include <chrono>
#include <fstream>
#include <memory>
#include <functional>

namespace ge {
namespace debug {

enum class ExportFormat {
    JSON,
    ChromeTracing,
    CSV,
    Binary
};

struct SessionMetadata {
    std::string sessionName;
    std::string applicationName;
    std::string applicationVersion;
    std::string platform;
    std::string architecture;
    std::string engineVersion;
    int64_t startTimestamp = 0;
    int64_t endTimestamp = 0;
    int frameCount = 0;
    double duration = 0.0;
};

struct SessionFrameData {
    int frameNumber = 0;
    int64_t timestamp = 0;
    double frameTimeMs = 0.0;
    double cpuTimeMs = 0.0;
    double gpuTimeMs = 0.0;
    int drawCalls = 0;
    int triangles = 0;
    int vertices = 0;
    size_t memoryUsed = 0;
    std::vector<std::string> scopeNames;
    std::vector<double> scopeTimes;
};

class TelemetrySession {
public:
    TelemetrySession();
    explicit TelemetrySession(const std::string& name);
    ~TelemetrySession();
    
    void Start();
    void Stop();
    void Reset();
    
    bool IsActive() const { return isActive_; }
    bool IsPaused() const { return isPaused_; }
    
    void Pause();
    void Resume();
    void TogglePause();
    
    void SetName(const std::string& name);
    const std::string& GetName() const { return name_; }
    
    void SetMetadata(const SessionMetadata& metadata);
    SessionMetadata& GetMetadata() { return metadata_; }
    const SessionMetadata& GetMetadata() const { return metadata_; }
    
    void AddFrameData(const SessionFrameData& frame);
    void AddFrameData(SessionFrameData&& frame);
    
    void SetMaxFrameDataSize(size_t maxFrames);
    size_t GetMaxFrameDataSize() const { return maxFrameDataSize_; }
    
    const std::vector<SessionFrameData>& GetFrameData() const { return frameData_; }
    const SessionFrameData* GetFrameData(int frameNumber) const;
    
    void SetSampleRate(int framesPerSecond);
    int GetSampleRate() const { return sampleRate_; }
    
    bool Export(const std::string& filepath, ExportFormat format);
    std::string ExportToString(ExportFormat format);
    
    bool Import(const std::string& filepath);
    bool ImportFromString(const std::string& data);
    
    std::string GetCacheDirectory() const;
    void SetCacheDirectory(const std::string& path);
    
    void Update();
    
    struct Statistics {
        int64_t totalFrames = 0;
        double avgFrameTime = 0.0;
        double minFrameTime = 0.0;
        double maxFrameTime = 0.0;
        int64_t totalDrawCalls = 0;
        int64_t totalTriangles = 0;
        size_t peakMemory = 0;
        double duration = 0.0;
    };
    
    const Statistics& GetStatistics() const { return stats_; }
    
    std::function<void(const std::string&)> onExportComplete;
    std::function<void(const std::string&)> onError;
    
    static std::unique_ptr<TelemetrySession> LoadFromFile(const std::string& filepath);
    static std::string GetDefaultCacheDirectory();
    
private:
    void CalculateStatistics();
    bool ExportJSON(std::ofstream& file);
    bool ExportChromeTracing(std::ofstream& file);
    bool ExportCSV(std::ofstream& file);
    bool ExportBinary(std::ofstream& file);
    
    std::string name_;
    SessionMetadata metadata_;
    std::vector<SessionFrameData> frameData_;
    
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point pauseTime_;
    
    bool isActive_ = false;
    bool isPaused_ = false;
    int sampleRate_ = 60;
    size_t maxFrameDataSize_ = 3600;
    std::string cacheDirectory_;
    
    Statistics stats_;
};

class TelemetryRecorder {
public:
    TelemetryRecorder();
    ~TelemetryRecorder();
    
    void Initialize();
    void Shutdown();
    
    void SetTargetSession(TelemetrySession* session);
    TelemetrySession* GetTargetSession() const { return targetSession_; }
    
    void RecordFrame();
    void SetRecordingEnabled(bool enabled);
    bool IsRecordingEnabled() const { return recordingEnabled_; }
    
    void SetAutoSave(bool enabled, int intervalSeconds = 60);
    bool IsAutoSaveEnabled() const { return autoSaveEnabled_; }
    int GetAutoSaveInterval() const { return autoSaveInterval_; }
    
    void SetMaxFileSize(size_t maxBytes);
    size_t GetMaxFileSize() const { return maxFileSize_; }
    
    void SetMaxStorageSize(size_t maxBytes);
    size_t GetMaxStorageSize() const { return maxStorageSize_; }
    
    void SaveSession(const std::string& filepath);
    void LoadSession(const std::string& filepath);
    
    std::vector<std::string> GetSavedSessions() const;
    void DeleteSession(const std::string& name);
    
    std::function<void(TelemetrySession*)> onSessionComplete;
    
private:
    void AutoSaveTimer();
    void EnforceStorageLimit();
    
    TelemetrySession* targetSession_ = nullptr;
    bool recordingEnabled_ = true;
    bool autoSaveEnabled_ = false;
    int autoSaveInterval_ = 60;
    size_t maxFileSize_ = 100 * 1024 * 1024;
    size_t maxStorageSize_ = 500 * 1024 * 1024;
    std::string storageDirectory_;
};

} // namespace debug
} // namespace ge

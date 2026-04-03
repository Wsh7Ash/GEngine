#pragma once

// ================================================================
//  TimelineView.h
//  Timeline visualization for events over time.
// ================================================================

#include "ProfilerEx.h"
#include "MetricTypes.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>

namespace ge {
namespace debug {

struct TimelineEvent {
    std::string name;
    int64_t startTime = 0;
    int64_t endTime = 0;
    int64_t timestamp = 0;
    std::string category;
    uint32_t threadId = 0;
    int64_t duration = 0;
    float percentage = 0.0f;
    
    bool operator<(const TimelineEvent& other) const {
        return startTime < other.startTime;
    }
};

struct TimelineTrack {
    std::string name;
    uint32_t threadId = 0;
    std::vector<TimelineEvent> events;
    bool isVisible = true;
    bool isCollapsed = false;
    float height = 30.0f;
    int colorIndex = 0;
};

struct TimelineConfig {
    double zoomLevel = 1.0;
    double minZoom = 0.1;
    double maxZoom = 100.0;
    double rowHeight = 24.0;
    double headerHeight = 30.0;
    double rulerHeight = 20.0;
    bool showRuler = true;
    bool showThreadLanes = true;
    bool showGPUTimeline = false;
    bool snapToEvents = true;
    double snapThreshold = 5.0;
};

class TimelineView {
public:
    TimelineView();
    explicit TimelineView(const std::string& name);
    ~TimelineView() = default;
    
    void SetProfiler(const ProfilerEx* profiler);
    const ProfilerEx* GetProfiler() const { return profiler_; }
    
    void SetVisible(bool visible) { isVisible_ = visible; }
    bool IsVisible() const { return isVisible_; }
    
    void SetTimeRange(int64_t start, int64_t end);
    void GetTimeRange(int64_t& start, int64_t& end) const;
    
    void SetWindowSize(double windowMs);
    double GetWindowSize() const { return windowSizeMs_; }
    
    void SetViewport(double startMs, double endMs);
    void GetViewport(double& startMs, double& endMs) const;
    
    void Zoom(double factor, double centerMs);
    void ZoomToFit();
    void Scroll(double deltaMs);
    
    void JumpToTime(int64_t timestamp);
    void CenterOnTime(int64_t timestamp);
    
    void AddTrack(const std::string& name, uint32_t threadId = 0);
    void RemoveTrack(const std::string& name);
    void SetTrackVisible(const std::string& name, bool visible);
    
    TimelineTrack* GetTrack(const std::string& name);
    const std::vector<TimelineTrack>& GetTracks() const { return tracks_; }
    
    void SetConfig(const TimelineConfig& config);
    TimelineConfig& GetConfig() { return config_; }
    const TimelineConfig& GetConfig() const { return config_; }
    
    void Render();
    
    void SetFilter(const std::string& filter);
    const std::string& GetFilter() const { return filter_; }
    
    void SetCategoryColors(const std::vector<std::pair<std::string, uint32_t>>& colors);
    uint32_t GetCategoryColor(const std::string& category) const;
    
    void SetSelectedEvent(const TimelineEvent* event);
    const TimelineEvent* GetSelectedEvent() const { return selectedEvent_; }
    
    void ExportToCSV(const std::string& filepath) const;
    
    std::function<void(const TimelineEvent&)> onEventSelected;
    std::function<void(int64_t, int64_t)> onTimeRangeSelected;
    
private:
    void BuildFromProfiler();
    void UpdateLayout();
    void FilterEvents();
    double TimeToPixel(int64_t time) const;
    int64_t PixelToTime(double pixel) const;
    
    std::string name_;
    const ProfilerEx* profiler_ = nullptr;
    std::vector<TimelineTrack> tracks_;
    
    int64_t timeStart_ = 0;
    int64_t timeEnd_ = 0;
    int64_t viewportStart_ = 0;
    int64_t viewportEnd_ = 0;
    double windowSizeMs_ = 100.0;
    
    bool isVisible_ = true;
    std::string filter_;
    TimelineConfig config_;
    
    std::unordered_map<std::string, uint32_t> categoryColors_;
    const TimelineEvent* selectedEvent_ = nullptr;
    
    std::vector<TimelineEvent> filteredEvents_;
};

class TimelineRuler {
public:
    TimelineRuler();
    ~TimelineRuler() = default;
    
    void SetTimeRange(int64_t start, int64_t end);
    void GetTimeRange(int64_t& start, int64_t& end) const;
    
    void SetViewport(double startMs, double endMs);
    void GetViewport(double& startMs, double& endMs) const;
    
    void SetHeight(double height);
    double GetHeight() const { return height_; }
    
    void SetVisible(bool visible) { isVisible_ = visible; }
    bool IsVisible() const { return isVisible_; }
    
    void Render();
    
    void SetBackgroundColor(uint32_t color) { backgroundColor_ = color; }
    uint32_t GetBackgroundColor() const { return backgroundColor_; }
    
    void SetTextColor(uint32_t color) { textColor_ = color; }
    uint32_t GetTextColor() const { return textColor_; }
    
private:
    void CalculateTickInterval();
    double GetOptimalTickInterval() const;
    std::string FormatTime(double ms) const;
    
    int64_t timeStart_ = 0;
    int64_t timeEnd_ = 0;
    double viewportStart_ = 0.0;
    double viewportEnd_ = 100.0;
    double height_ = 20.0;
    bool isVisible_ = true;
    
    double tickInterval_ = 10.0;
    uint32_t backgroundColor_ = 0xFF1A1A1A;
    uint32_t textColor_ = 0xFFFFFFFF;
};

class EventMarkers {
public:
    EventMarkers();
    ~EventMarkers() = default;
    
    void AddMarker(const std::string& name, int64_t timestamp, const std::string& label = "");
    void AddMarker(const std::string& name, int64_t start, int64_t end, const std::string& label = "");
    void RemoveMarker(const std::string& name);
    void ClearMarkers();
    
    const std::vector<TimelineEvent>& GetMarkers() const { return markers_; }
    
    void SetVisible(bool visible) { isVisible_ = visible; }
    bool IsVisible() const { return isVisible_; }
    
    void Render(TimelineView& timeline);
    
    void SetMarkerColor(const std::string& name, uint32_t color);
    uint32_t GetMarkerColor(const std::string& name) const;
    
private:
    std::vector<TimelineEvent> markers_;
    std::unordered_map<std::string, uint32_t> markerColors_;
    bool isVisible_ = true;
};

} // namespace debug
} // namespace ge

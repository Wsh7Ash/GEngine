#pragma once

// ================================================================
//  FrameBudgetProfiler.h
//  Frame budget tracking with pass-level profiling.
// ================================================================

#include "ProfilerEx.h"
#include "FramePasses.h"
#include "MetricRegistry.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <optional>

namespace ge {
namespace debug {

struct PassTiming {
    const char* name = "";
    double budgetMs = 0.0;
    double actualMs = 0.0;
    double overBudgetMs = 0.0;
    double percentOfFrame = 0.0;
    bool exceeded = false;
    bool warnEnabled = true;
};

struct FrameBudgetReport {
    double totalFrameMs = 0.0;
    double frameBudgetMs = 0.0;
    double frameOverBudgetMs = 0.0;
    double overBudgetPercent = 0.0;
    std::vector<PassTiming> passTimings;
    std::vector<std::string> warnings;
    int64_t frameNumber = 0;
};

class FrameBudgetProfiler {
public:
    FrameBudgetProfiler();
    ~FrameBudgetProfiler();

    void Initialize();
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void BeginPass(const char* passName);
    void EndPass(const char* passName);

    void SetFrameBudget(double budgetMs);
    double GetFrameBudget() const { return frameBudgetMs_; }

    void SetPassBudget(const char* passName, double budgetMs);
    double GetPassBudget(const char* passName) const;

    void SetPassWarningEnabled(const char* passName, bool enabled);
    bool IsPassWarningEnabled(const char* passName) const;

    bool IsFrameOverBudget() const { return isOverBudget_; }
    double GetFrameOverBudgetPercent() const { return overBudgetPercent_; }

    const FrameBudgetReport& GetCurrentReport() const { return currentReport_; }
    const std::vector<FrameBudgetReport>& GetRecentReports() const { return recentReports_; }

    void SetMaxReports(size_t max);
    size_t GetMaxReports() const { return maxReports_; }

    void SetProfilerEx(ProfilerEx* profiler) { profilerEx_ = profiler; }
    ProfilerEx* GetProfilerEx() const { return profilerEx_; }

    void ExportMetrics(MetricRegistry* registry);

    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }

private:
    void CalculateReport();
    void LogWarnings();

    double GetCurrentTimeMs() const;

    bool enabled_ = false;
    double frameBudgetMs_ = kDefaultFrameBudgetMs;

    std::vector<PassTiming> passTimings_;
    std::unordered_map<std::string, double> passBudgets_;
    std::unordered_map<std::string, bool> passWarnings_;
    std::unordered_map<std::string, std::pair<double, bool>> activePassStack_;

    FrameBudgetReport currentReport_;
    std::vector<FrameBudgetReport> recentReports_;
    size_t maxReports_ = 60;

    bool isOverBudget_ = false;
    double overBudgetPercent_ = 0.0;

    int64_t frameNumber_ = 0;

    ProfilerEx* profilerEx_ = nullptr;

    std::chrono::steady_clock::time_point frameStartTime_;
    std::chrono::steady_clock::time_point passStartTime_;
};

class ScopedPassProfile {
public:
    ScopedPassProfile(const char* passName);
    ~ScopedPassProfile();

private:
    const char* passName_;
};

} // namespace debug
} // namespace ge

#if defined(GE_DEBUG) || defined(GE_ENABLE_PROFILING)
    #define GE_PROFILE_PASS(name) ge::debug::ScopedPassProfile _ge_pass_profile(name)
    #define GE_PROFILE_PASS_CUSTOM(name, budgetMs) \
        do { ge::debug::FrameBudgetProfiler::Get().SetPassBudget(name, budgetMs); } while(0)
#else
    #define GE_PROFILE_PASS(name)
    #define GE_PROFILE_PASS_CUSTOM(name, budgetMs)
#endif
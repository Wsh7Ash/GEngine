#include "FrameBudgetProfiler.h"
#include "../debug/log.h"
#include <algorithm>
#include <cstring>

namespace ge {
namespace debug {

FrameBudgetProfiler::FrameBudgetProfiler() {
    for (size_t i = 0; i < kDefaultPassCount; ++i) {
        passBudgets_[kDefaultFramePasses[i].name] = kDefaultFramePasses[i].budgetMs;
        passWarnings_[kDefaultFramePasses[i].name] = kDefaultFramePasses[i].warnOnOverBudget;
    }
}

FrameBudgetProfiler::~FrameBudgetProfiler() {
    Shutdown();
}

void FrameBudgetProfiler::Initialize() {
    if (enabled_) return;
    enabled_ = true;
    frameNumber_ = 0;
    GE_LOG_INFO("FrameBudgetProfiler initialized with {}ms frame budget", frameBudgetMs_);
}

void FrameBudgetProfiler::Shutdown() {
    if (!enabled_) return;
    enabled_ = false;
}

void FrameBudgetProfiler::BeginFrame() {
    if (!enabled_) return;

    frameStartTime_ = std::chrono::steady_clock::now();
    frameNumber_++;

    passTimings_.clear();
    activePassStack_.clear();

    if (profilerEx_) {
        profilerEx_->BeginFrame();
    }
}

void FrameBudgetProfiler::EndFrame() {
    if (!enabled_) return;

    auto now = std::chrono::steady_clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(now - frameStartTime_).count();

    for (auto& [passName, startTime] : activePassStack_) {
        auto passEnd = std::chrono::steady_clock::now();
        double passMs = std::chrono::duration<double, std::milli>(passEnd - startTime.second).count();

        PassTiming timing;
        timing.name = passName;
        timing.budgetMs = GetPassBudget(passName);
        timing.actualMs = passMs;
        timing.overBudgetMs = std::max(0.0, passMs - timing.budgetMs);
        timing.percentOfFrame = (totalMs > 0.0) ? (passMs / totalMs) * 100.0 : 0.0;
        timing.exceeded = passMs > timing.budgetMs;
        timing.warnEnabled = IsPassWarningEnabled(passName);

        passTimings_.push_back(timing);
    }

    CalculateReport();

    if (profilerEx_) {
        profilerEx_->EndFrame();
    }

    isOverBudget_ = currentReport_.overBudgetPercent > 0.0;
    overBudgetPercent_ = currentReport_.overBudgetPercent;

    if (isOverBudget_) {
        LogWarnings();
    }
}

void FrameBudgetProfiler::BeginPass(const char* passName) {
    if (!enabled_ || !passName) return;

    auto now = std::chrono::steady_clock::now();
    activePassStack_[passName] = {GetPassBudget(passName), now};
}

void FrameBudgetProfiler::EndPass(const char* passName) {
    if (!enabled_ || !passName) return;

    auto it = activePassStack_.find(passName);
    if (it == activePassStack_.end()) return;

    auto now = std::chrono::steady_clock::now();
    double passMs = std::chrono::duration<double, std::milli>(now - it->second.second).count();

    PassTiming timing;
    timing.name = passName;
    timing.budgetMs = GetPassBudget(passName);
    timing.actualMs = passMs;
    timing.overBudgetMs = std::max(0.0, passMs - timing.budgetMs);

    double totalFrameMs = std::chrono::duration<double, std::milli>(now - frameStartTime_).count();
    timing.percentOfFrame = (totalFrameMs > 0.0) ? (passMs / totalFrameMs) * 100.0 : 0.0;
    timing.exceeded = passMs > timing.budgetMs;
    timing.warnEnabled = IsPassWarningEnabled(passName);

    passTimings_.push_back(timing);
    activePassStack_.erase(it);
}

void FrameBudgetProfiler::SetFrameBudget(double budgetMs) {
    frameBudgetMs_ = budgetMs;
}

void FrameBudgetProfiler::SetPassBudget(const char* passName, double budgetMs) {
    if (passName) {
        passBudgets_[passName] = budgetMs;
    }
}

double FrameBudgetProfiler::GetPassBudget(const char* passName) const {
    if (!passName) return 0.0;

    auto it = passBudgets_.find(passName);
    if (it != passBudgets_.end()) {
        return it->second;
    }

    for (size_t i = 0; i < kDefaultPassCount; ++i) {
        if (std::strcmp(kDefaultFramePasses[i].name, passName) == 0) {
            return kDefaultFramePasses[i].budgetMs;
        }
    }

    return 2.0;
}

void FrameBudgetProfiler::SetPassWarningEnabled(const char* passName, bool enabled) {
    if (passName) {
        passWarnings_[passName] = enabled;
    }
}

bool FrameBudgetProfiler::IsPassWarningEnabled(const char* passName) const {
    if (!passName) return false;

    auto it = passWarnings_.find(passName);
    if (it != passWarnings_.end()) {
        return it->second;
    }

    for (size_t i = 0; i < kDefaultPassCount; ++i) {
        if (std::strcmp(kDefaultFramePasses[i].name, passName) == 0) {
            return kDefaultFramePasses[i].warnOnOverBudget;
        }
    }

    return true;
}

void FrameBudgetProfiler::SetMaxReports(size_t max) {
    maxReports_ = max;
    if (recentReports_.size() > maxReports_) {
        recentReports_.resize(maxReports_);
    }
}

void FrameBudgetProfiler::ExportMetrics(MetricRegistry* registry) {
    if (!registry) return;

    auto* frameTime = registry->RegisterTimer("Frame.Time", "Total frame time", "Performance");
    if (frameTime) {
        frameTime->Set(currentReport_.totalFrameMs);
    }

    auto* frameBudget = registry->RegisterGauge("Frame.Budget", "Frame budget", "Performance");
    if (frameBudget) {
        frameBudget->Set(frameBudgetMs_);
    }

    auto* overBudget = registry->RegisterGauge("Frame.OverBudget", "Over budget percentage", "Performance");
    if (overBudget) {
        overBudget->Set(currentReport_.overBudgetPercent);
    }

    for (const auto& timing : currentReport_.passTimings) {
        std::string timerName = std::string("Pass.") + timing.name + ".Time";
        auto* timer = registry->RegisterTimer(timerName.c_str(), timing.name, "Performance");
        if (timer) {
            timer->Set(timing.actualMs);
        }

        std::string gaugeName = std::string("Pass.") + timing.name + ".Percent";
        auto* gauge = registry->RegisterGauge(gaugeName.c_str(), timing.name, "Performance");
        if (gauge) {
            gauge->Set(timing.percentOfFrame);
        }
    }
}

void FrameBudgetProfiler::CalculateReport() {
    FrameBudgetReport report;
    report.frameNumber = frameNumber_;
    report.frameBudgetMs = frameBudgetMs_;

    double totalPassTime = 0.0;
    for (const auto& timing : passTimings_) {
        report.passTimings.push_back(timing);
        totalPassTime += timing.actualMs;
    }

    report.totalFrameMs = totalPassTime;
    report.frameOverBudgetMs = std::max(0.0, totalPassTime - frameBudgetMs_);
    report.overBudgetPercent = (frameBudgetMs_ > 0.0) ? ((totalPassTime - frameBudgetMs_) / frameBudgetMs_) * 100.0 : 0.0;

    currentReport_ = report;

    recentReports_.push_back(report);
    if (recentReports_.size() > maxReports_) {
        recentReports_.erase(recentReports_.begin());
    }
}

void FrameBudgetProfiler::LogWarnings() {
    for (const auto& timing : currentReport_.passTimings) {
        if (timing.exceeded && timing.warnEnabled) {
            GE_LOG_WARN("Frame pass '{}' exceeded budget: {:.2f}ms / {:.2f}ms ({:.1f}%)",
                        timing.name, timing.actualMs, timing.budgetMs, timing.percentOfFrame);
        }
    }

    if (currentReport_.overBudgetPercent > 0.0) {
        GE_LOG_WARN("Frame {:.0f} over budget: {:.2f}ms / {:.2f}ms ({:.1f}%)",
                    frameNumber_, currentReport_.totalFrameMs, frameBudgetMs_, currentReport_.overBudgetPercent);
    }
}

double FrameBudgetProfiler::GetCurrentTimeMs() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(now - frameStartTime_).count();
}

ScopedPassProfile::ScopedPassProfile(const char* passName)
    : passName_(passName) {
    FrameBudgetProfiler::Get().BeginPass(passName_);
}

ScopedPassProfile::~ScopedPassProfile() {
    FrameBudgetProfiler::Get().EndPass(passName_);
}

} // namespace debug
} // namespace ge
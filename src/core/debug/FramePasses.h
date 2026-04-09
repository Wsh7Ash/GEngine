#pragma once

// ================================================================
//  FramePasses.h
//  Predefined frame pass definitions with budgets.
// ================================================================

#include <cstddef>

namespace ge {
namespace debug {

struct FramePassDef {
    const char* name;
    double budgetMs;
    bool warnOnOverBudget;
};

constexpr FramePassDef kDefaultFramePasses[] = {
    {"Physics", 8.0, true},
    {"Script", 4.0, true},
    {"Render", 12.0, true},
    {"Network", 2.0, false},
    {"Audio", 1.0, false},
    {"Particle", 2.0, false},
    {"Animation", 2.0, false},
    {"Total", 16.67, true}
};

constexpr size_t kDefaultPassCount = sizeof(kDefaultFramePasses) / sizeof(kDefaultFramePasses[0]);

constexpr double kDefaultFrameBudgetMs = 16.67;

constexpr double kDefaultFrameBudgetUs = kDefaultFrameBudgetMs * 1000.0;

class FramePassRegistry {
public:
    static FramePassRegistry& Get();

    void RegisterPass(const char* name, double budgetMs, bool warnOnOverBudget = true);
    void ClearPasses();

    size_t GetPassCount() const { return passCount_; }

    const FramePassDef* GetPass(size_t index) const;
    const FramePassDef* GetPass(const char* name) const;

    double GetTotalBudgetMs() const { return totalBudgetMs_; }

private:
    FramePassRegistry();
    ~FramePassRegistry();

    void InitializeDefaults();

    struct PassEntry {
        const char* name;
        double budgetMs;
        bool warnOnOverBudget;
    };

    PassEntry passes_[16];
    size_t passCount_ = 0;
    double totalBudgetMs_ = 0.0;
};

} // namespace debug
} // namespace ge
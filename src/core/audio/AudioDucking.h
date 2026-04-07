#pragma once

#include "AudioCategory.h"
#include <vector>
#include <unordered_map>
#include <functional>

namespace ge {
namespace audio {

struct DuckingRule {
    AudioCategory triggerCategory;
    AudioCategory targetCategory;
    float duckAmount = 0.3f;
    float fadeInTime = 0.1f;
    float fadeOutTime = 0.5f;
    float holdTime = 0.3f;
    bool enabled = true;

    DuckingRule() = default;

    DuckingRule(AudioCategory trigger, AudioCategory target, float duck = 0.3f)
        : triggerCategory(trigger), targetCategory(target), duckAmount(duck) {}
};

struct ActiveDucking {
    AudioCategory targetCategory;
    float targetVolume;
    float currentVolume;
    float timer;
    bool fadingIn;
    DuckingRule rule;
};

class AudioDuckingManager {
public:
    static AudioDuckingManager& Get();

    AudioDuckingManager();
    ~AudioDuckingManager();

    void Initialize();

    void AddRule(const DuckingRule& rule);
    void RemoveRule(AudioCategory trigger, AudioCategory target);
    void ClearRules();

    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }

    void OnSoundStarted(AudioCategory category);
    void OnSoundFinished(AudioCategory category);

    void Update(float dt);

    float GetEffectiveVolume(AudioCategory category) const;
    float GetDuckingReduction(AudioCategory category) const;

    std::vector<DuckingRule> GetRules() const { return rules_; }

private:
    bool enabled_ = true;
    std::vector<DuckingRule> rules_;
    std::vector<ActiveDucking> activeDuckings_;

    DuckingRule* FindMatchingRule(AudioCategory trigger, AudioCategory target);
    void StartDucking(const DuckingRule& rule);
    void StopDucking(AudioCategory target);
};

void SetDefaultDuckingRules();

} // namespace audio
} // namespace ge
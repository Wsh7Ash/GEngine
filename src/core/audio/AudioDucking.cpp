#include "AudioDucking.h"
#include "../debug/log.h"
#include <algorithm>

namespace ge {
namespace audio {

AudioDuckingManager& AudioDuckingManager::Get() {
    static AudioDuckingManager instance;
    return instance;
}

AudioDuckingManager::AudioDuckingManager() = default;
AudioDuckingManager::~AudioDuckingManager() = default;

void AudioDuckingManager::Initialize() {
    activeDuckings_.clear();
    GE_LOG_INFO("AudioDuckingManager: Initialized");
}

void AudioDuckingManager::AddRule(const DuckingRule& rule) {
    rules_.push_back(rule);
    GE_LOG_INFO("AudioDuckingManager: Added ducking rule %s -> %s (%.0f%%)",
        CategoryToString(rule.triggerCategory),
        CategoryToString(rule.targetCategory),
        rule.duckAmount * 100.0f);
}

void AudioDuckingManager::RemoveRule(AudioCategory trigger, AudioCategory target) {
    rules_.erase(
        std::remove_if(rules_.begin(), rules_.end(),
            [trigger, target](const DuckingRule& r) {
                return r.triggerCategory == trigger && r.targetCategory == target;
            }),
        rules_.end()
    );
}

void AudioDuckingManager::ClearRules() {
    rules_.clear();
    activeDuckings_.clear();
}

DuckingRule* AudioDuckingManager::FindMatchingRule(AudioCategory trigger, AudioCategory target) {
    for (auto& rule : rules_) {
        if (rule.enabled && rule.triggerCategory == trigger && rule.targetCategory == target) {
            return &rule;
        }
    }
    return nullptr;
}

void AudioDuckingManager::StartDucking(const DuckingRule& rule) {
    for (auto& active : activeDuckings_) {
        if (active.targetCategory == rule.targetCategory) {
            active.timer = rule.holdTime + rule.fadeInTime;
            active.targetVolume = rule.duckAmount;
            active.rule = rule;
            active.fadingIn = true;
            return;
        }
    }

    ActiveDucking newDucking;
    newDucking.targetCategory = rule.targetCategory;
    newDucking.targetVolume = rule.duckAmount;
    newDucking.currentVolume = 1.0f;
    newDucking.timer = rule.holdTime + rule.fadeInTime;
    newDucking.fadingIn = true;
    newDucking.rule = rule;
    activeDuckings_.push_back(newDucking);
}

void AudioDuckingManager::StopDucking(AudioCategory target) {
    for (auto& active : activeDuckings_) {
        if (active.targetCategory == target) {
            active.fadingIn = false;
        }
    }
}

void AudioDuckingManager::OnSoundStarted(AudioCategory category) {
    if (!enabled_) return;

    for (auto& rule : rules_) {
        if (rule.enabled && rule.triggerCategory == category) {
            StartDucking(rule);
        }
    }
}

void AudioDuckingManager::OnSoundFinished(AudioCategory category) {
    bool anyPlaying = false;
    for (const auto& rule : rules_) {
        if (rule.triggerCategory == category) {
            anyPlaying = true;
            break;
        }
    }

    if (!anyPlaying) {
        for (const auto& rule : rules_) {
            if (rule.triggerCategory == category) {
                StopDucking(rule.targetCategory);
            }
        }
    }
}

void AudioDuckingManager::Update(float dt) {
    for (auto& active : activeDuckings_) {
        if (active.fadingIn) {
            if (active.timer > 0.0f) {
                active.timer -= dt;
            } else {
                float t = std::min(1.0f, dt / active.rule.fadeInTime);
                active.currentVolume = active.currentVolume + (active.targetVolume - active.currentVolume) * t;
            }
        } else {
            float t = std::min(1.0f, dt / active.rule.fadeOutTime);
            active.currentVolume = active.currentVolume + (1.0f - active.currentVolume) * t;
            if (active.currentVolume >= 0.99f) {
                active.currentVolume = 1.0f;
            }
        }
    }

    activeDuckings_.erase(
        std::remove_if(activeDuckings_.begin(), activeDuckings_.end(),
            [](const ActiveDucking& ad) { return ad.currentVolume >= 0.99f; }),
        activeDuckings_.end()
    );
}

float AudioDuckingManager::GetEffectiveVolume(AudioCategory category) const {
    for (const auto& active : activeDuckings_) {
        if (active.targetCategory == category) {
            return active.currentVolume;
        }
    }
    return 1.0f;
}

float AudioDuckingManager::GetDuckingReduction(AudioCategory category) const {
    return 1.0f - GetEffectiveVolume(category);
}

void SetDefaultDuckingRules() {
    auto& manager = AudioDuckingManager::Get();
    manager.ClearRules();

    manager.AddRule(DuckingRule(AudioCategory::SFX, AudioCategory::Music, 0.3f));
    manager.AddRule(DuckingRule(AudioCategory::SFX, AudioCategory::Ambient, 0.4f));
    manager.AddRule(DuckingRule(AudioCategory::Voice, AudioCategory::Music, 0.5f));
    manager.AddRule(DuckingRule(AudioCategory::Voice, AudioCategory::Ambient, 0.6f));
    manager.AddRule(DuckingRule(AudioCategory::UI, AudioCategory::Music, 0.8f));
    manager.AddRule(DuckingRule(AudioCategory::UI, AudioCategory::Ambient, 0.9f));
}

} // namespace audio
} // namespace ge
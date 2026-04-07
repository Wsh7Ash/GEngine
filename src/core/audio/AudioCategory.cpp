#include "AudioCategory.h"
#include "../debug/log.h"

namespace ge {
namespace audio {

AudioCategoryManager& AudioCategoryManager::Get() {
    static AudioCategoryManager instance;
    return instance;
}

AudioCategoryManager::AudioCategoryManager() {
    categoryInfos_ = {
        {AudioCategory::Master, {"Master", 1.0f, false, false, 0}},
        {AudioCategory::SFX, {"SFX", 1.0f, false, true, 16}},
        {AudioCategory::Music, {"Music", 0.8f, false, false, 4}},
        {AudioCategory::Voice, {"Voice", 1.0f, false, true, 8}},
        {AudioCategory::Ambient, {"Ambient", 0.7f, false, false, 8}},
        {AudioCategory::UI, {"UI", 1.0f, false, false, 8}}
    };
}

AudioCategoryManager::~AudioCategoryManager() = default;

void AudioCategoryManager::Initialize() {
    for (const auto& [cat, info] : categoryInfos_) {
        categoryVolumes_[cat] = info.defaultVolume;
        categoryMutes_[cat] = info.defaultMuted;
        categoryPitches_[cat] = 1.0f;
        activeSoundCounts_[cat] = 0;
    }
    GE_LOG_INFO("AudioCategoryManager: Initialized");
}

void AudioCategoryManager::SetCategoryVolume(AudioCategory category, float volume) {
    categoryVolumes_[category] = std::max(0.0f, std::min(2.0f, volume));
}

float AudioCategoryManager::GetCategoryVolume(AudioCategory category) const {
    auto it = categoryVolumes_.find(category);
    if (it != categoryVolumes_.end()) {
        return it->second;
    }
    return 1.0f;
}

void AudioCategoryManager::SetCategoryMute(AudioCategory category, bool muted) {
    categoryMutes_[category] = muted;
}

bool AudioCategoryManager::IsCategoryMuted(AudioCategory category) const {
    auto it = categoryMutes_.find(category);
    return it != categoryMutes_.end() && it->second;
}

void AudioCategoryManager::SetCategoryPitch(AudioCategory category, float pitch) {
    categoryPitches_[category] = std::max(0.1f, std::min(4.0f, pitch));
}

float AudioCategoryManager::GetCategoryPitch(AudioCategory category) const {
    auto it = categoryPitches_.find(category);
    if (it != categoryPitches_.end()) {
        return it->second;
    }
    return 1.0f;
}

bool AudioCategoryManager::CanPlaySound(AudioCategory category) {
    auto it = categoryInfos_.find(category);
    if (it == categoryInfos_.end()) return true;

    int maxSounds = it->second.maxConcurrentSounds;
    if (maxSounds <= 0) return true;

    int active = activeSoundCounts_[category];
    return active < maxSounds;
}

void AudioCategoryManager::OnSoundFinished(AudioCategory category) {
    auto it = activeSoundCounts_.find(category);
    if (it != activeSoundCounts_.end() && it->second > 0) {
        it->second--;
    }
}

const AudioCategoryInfo* AudioCategoryManager::GetCategoryInfo(AudioCategory category) const {
    auto it = categoryInfos_.find(category);
    if (it != categoryInfos_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::string AudioCategoryManager::GetCategoryName(AudioCategory category) const {
    auto it = categoryInfos_.find(category);
    if (it != categoryInfos_.end()) {
        return it->second.displayName;
    }
    return "Unknown";
}

AudioCategory CategoryFromString(const std::string& str) {
    if (str == "master" || str == "Master") return AudioCategory::Master;
    if (str == "sfx" || str == "SFX") return AudioCategory::SFX;
    if (str == "music" || str == "Music") return AudioCategory::Music;
    if (str == "voice" || str == "Voice") return AudioCategory::Voice;
    if (str == "ambient" || str == "Ambient") return AudioCategory::Ambient;
    if (str == "ui" || str == "UI") return AudioCategory::UI;
    return AudioCategory::SFX;
}

const char* CategoryToString(AudioCategory category) {
    switch (category) {
        case AudioCategory::Master: return "Master";
        case AudioCategory::SFX: return "SFX";
        case AudioCategory::Music: return "Music";
        case AudioCategory::Voice: return "Voice";
        case AudioCategory::Ambient: return "Ambient";
        case AudioCategory::UI: return "UI";
        default: return "Unknown";
    }
}

} // namespace audio
} // namespace ge
#pragma once

#include <string>
#include <unordered_map>

namespace ge {
namespace audio {

enum class AudioCategory {
    Master,
    SFX,
    Music,
    Voice,
    Ambient,
    UI
};

struct AudioCategoryInfo {
    std::string displayName;
    float defaultVolume = 1.0f;
    bool defaultMuted = false;
    bool supportsSpatialization = true;
    int maxConcurrentSounds = 16;
};

class AudioCategoryManager {
public:
    static AudioCategoryManager& Get();

    AudioCategoryManager();
    ~AudioCategoryManager();

    void Initialize();

    void SetCategoryVolume(AudioCategory category, float volume);
    float GetCategoryVolume(AudioCategory category) const;

    void SetCategoryMute(AudioCategory category, bool muted);
    bool IsCategoryMuted(AudioCategory category) const;

    void SetCategoryPitch(AudioCategory category, float pitch);
    float GetCategoryPitch(AudioCategory category) const;

    bool CanPlaySound(AudioCategory category);
    void OnSoundFinished(AudioCategory category);

    const AudioCategoryInfo* GetCategoryInfo(AudioCategory category) const;
    std::string GetCategoryName(AudioCategory category) const;

private:
    std::unordered_map<AudioCategory, float> categoryVolumes_;
    std::unordered_map<AudioCategory, bool> categoryMutes_;
    std::unordered_map<AudioCategory, float> categoryPitches_;
    std::unordered_map<AudioCategory, int> activeSoundCounts_;
    std::unordered_map<AudioCategory, AudioCategoryInfo> categoryInfos_;
};

AudioCategory CategoryFromString(const std::string& str);
const char* CategoryToString(AudioCategory category);

} // namespace audio
} // namespace ge
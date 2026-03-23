#pragma once

#include <string>

// Forward-declare miniaudio types to avoid pulling the full header
struct ma_engine;

namespace ge {
namespace audio {

/**
 * @brief Singleton wrapper around MiniAudio's ma_engine.
 *
 * Call Init() once at startup and Shutdown() before exit.
 */
class AudioEngine {
public:
    static void Init();
    static void Shutdown();
    static bool IsInitialized();

    /// Fire-and-forget one-shot sound.
    static void PlaySound(const std::string& filePath, float volume = 1.0f);

    /// Returns the raw ma_engine pointer for advanced usage (e.g. ma_sound).
    static ma_engine* GetEngine();

private:
    static ma_engine* s_Engine;
    static bool       s_Initialized;
};

} // namespace audio
} // namespace ge

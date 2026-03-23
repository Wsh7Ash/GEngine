#include "AudioEngine.h"
#include "../debug/log.h"
#include <miniaudio.h>

namespace ge {
namespace audio {

ma_engine* AudioEngine::s_Engine      = nullptr;
bool       AudioEngine::s_Initialized = false;

void AudioEngine::Init()
{
    if (s_Initialized) return;

    s_Engine = new ma_engine();
    ma_result result = ma_engine_init(nullptr, s_Engine);
    if (result != MA_SUCCESS) {
        GE_LOG_ERROR("AudioEngine: Failed to initialize MiniAudio engine (error %d)", (int)result);
        delete s_Engine;
        s_Engine = nullptr;
        return;
    }

    s_Initialized = true;
    GE_LOG_INFO("AudioEngine: Initialized successfully.");
}

void AudioEngine::Shutdown()
{
    if (!s_Initialized) return;

    ma_engine_uninit(s_Engine);
    delete s_Engine;
    s_Engine = nullptr;
    s_Initialized = false;
    GE_LOG_INFO("AudioEngine: Shut down.");
}

bool AudioEngine::IsInitialized()
{
    return s_Initialized;
}

void AudioEngine::PlaySound(const std::string& filePath, float volume)
{
    if (!s_Initialized) {
        GE_LOG_WARNING("AudioEngine: Cannot play sound — engine not initialized.");
        return;
    }
    ma_engine_play_sound(s_Engine, filePath.c_str(), nullptr);
    (void)volume;
}

ma_engine* AudioEngine::GetEngine()
{
    return s_Engine;
}

} // namespace audio
} // namespace ge

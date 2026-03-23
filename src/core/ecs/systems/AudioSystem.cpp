#include "AudioSystem.h"
#include "../../audio/AudioEngine.h"
#include "../../ge_core.h"
#include "../components/AudioSourceComponent.h"
#include "../components/AudioListenerComponent.h"
#include <miniaudio.h>

namespace ge {
namespace ecs {

void AudioSystem::Init()
{
    audio::AudioEngine::Init();
}

void AudioSystem::Update(World& world, float dt)
{
    if (!audio::AudioEngine::IsInitialized()) return;

    ma_engine* engine = audio::AudioEngine::GetEngine();

    // ── Listener position (from the active AudioListenerComponent) ──
    Math::Vec3f listenerPos = Math::Vec3f::Zero();
    for (auto entity : world.Query<AudioListenerComponent, TransformComponent>())
    {
        auto& listener = world.GetComponent<AudioListenerComponent>(entity);
        if (listener.IsActive) {
            auto& xform = world.GetComponent<TransformComponent>(entity);
            listenerPos = xform.position;
            ma_engine_listener_set_position(engine, 0,
                listenerPos.x, listenerPos.y, listenerPos.z);
            break; // Only one active listener
        }
    }

    // ── Audio sources ──
    for (auto entity : world.Query<AudioSourceComponent>())
    {
        auto& src = world.GetComponent<AudioSourceComponent>(entity);

        // PlayOnAwake: start playback once
        if (src.PlayOnAwake && !src.HasStarted && !src.FilePath.empty())
        {
            auto* sound = new ma_sound();
            ma_result r = ma_sound_init_from_file(engine, src.FilePath.c_str(),
                MA_SOUND_FLAG_DECODE, nullptr, nullptr, sound);

            if (r == MA_SUCCESS) {
                ma_sound_set_volume(sound, src.Volume);
                ma_sound_set_pitch(sound, src.Pitch);
                ma_sound_set_looping(sound, src.Loop ? MA_TRUE : MA_FALSE);

                if (src.Is3D && world.HasComponent<TransformComponent>(entity)) {
                    auto& xform = world.GetComponent<TransformComponent>(entity);
                    ma_sound_set_position(sound, xform.position.x, xform.position.y, xform.position.z);
                    ma_sound_set_min_distance(sound, src.MinDistance);
                    ma_sound_set_max_distance(sound, src.MaxDistance);
                } else {
                    ma_sound_set_spatialization_enabled(sound, MA_FALSE);
                }

                ma_sound_start(sound);
                src.InternalSound = sound;
                src.IsPlaying = true;
            } else {
                GE_LOG_WARNING("AudioSystem: Failed to load sound '%s'", src.FilePath.c_str());
                delete sound;
            }

            src.HasStarted = true;
        }

        // Update 3D position for playing sounds
        if (src.IsPlaying && src.InternalSound && src.Is3D &&
            world.HasComponent<TransformComponent>(entity))
        {
            auto& xform = world.GetComponent<TransformComponent>(entity);
            auto* sound = static_cast<ma_sound*>(src.InternalSound);
            ma_sound_set_position(sound, xform.position.x, xform.position.y, xform.position.z);
        }

        // Check if a non-looping sound has finished
        if (src.IsPlaying && src.InternalSound && !src.Loop)
        {
            auto* sound = static_cast<ma_sound*>(src.InternalSound);
            if (ma_sound_at_end(sound)) {
                ma_sound_uninit(sound);
                delete sound;
                src.InternalSound = nullptr;
                src.IsPlaying = false;
            }
        }
    }
}

void AudioSystem::Shutdown()
{
    audio::AudioEngine::Shutdown();
}

} // namespace ecs
} // namespace ge

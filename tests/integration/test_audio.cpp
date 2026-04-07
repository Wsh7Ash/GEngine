#include "../catch_amalgamated.hpp"

#include "../../src/core/ecs/components/AudioSourceComponent.h"
#include "../../src/core/ecs/components/AudioListenerComponent.h"
#include "../../src/core/audio/AudioCategory.h"
#include "../../src/core/audio/AudioDucking.h"
#include <cmath>

using namespace ge::ecs;
using namespace ge::audio;

TEST_CASE("Audio - Linear Distance Attenuation", "[audio]")
{
    float result = LinearDistanceAttenuation(0.5f, 1.0f, 10.0f, 1.0f);
    REQUIRE(result == 1.0f);
    
    result = LinearDistanceAttenuation(10.0f, 1.0f, 10.0f, 1.0f);
    REQUIRE(result == 0.0f);
    
    result = LinearDistanceAttenuation(5.5f, 1.0f, 10.0f, 1.0f);
    REQUIRE(result > 0.4f);
    REQUIRE(result < 0.6f);
}

TEST_CASE("Audio - Inverse Distance Attenuation", "[audio]")
{
    float result = InverseDistanceAttenuation(1.0f, 1.0f, 1.0f);
    REQUIRE(result == 1.0f);
    
    result = InverseDistanceAttenuation(11.0f, 1.0f, 1.0f);
    REQUIRE(result < 0.1f);
    REQUIRE(result > 0.0f);
    
    result = InverseDistanceAttenuation(5.5f, 1.0f, 1.0f);
    REQUIRE(result > 0.15f);
    REQUIRE(result < 0.2f);
}

TEST_CASE("Audio - Exponential Distance Attenuation", "[audio]")
{
    float result = ExponentialDistanceAttenuation(1.0f, 1.0f, 1.0f);
    REQUIRE(result == 1.0f);
    
    result = ExponentialDistanceAttenuation(10.0f, 1.0f, 1.0f);
    REQUIRE(result == 0.1f);
    
    result = ExponentialDistanceAttenuation(5.0f, 1.0f, 1.0f);
    REQUIRE(result > 0.1f);
    REQUIRE(result < 0.3f);
}

TEST_CASE("Audio - Logarithmic Distance Attenuation", "[audio]")
{
    float result = LogarithmicDistanceAttenuation(1.0f, 1.0f, 1.0f);
    REQUIRE(result == 1.0f);
    
    result = LogarithmicDistanceAttenuation(10.0f, 1.0f, 1.0f);
    REQUIRE(result < 0.5f);
    REQUIRE(result > 0.0f);
    
    result = LogarithmicDistanceAttenuation(100.0f, 1.0f, 1.0f);
    REQUIRE(result < 0.3f);
}

TEST_CASE("Audio - CalculateAttenuation Method", "[audio]")
{
    AudioSourceComponent src;
    src.ReferenceDistance = 1.0f;
    src.MaxDistance = 10.0f;
    src.RolloffFactor = 1.0f;
    
    src.DistanceModel = AudioDistanceModel::Linear;
    REQUIRE(src.CalculateAttenuation(0.5f) == 1.0f);
    REQUIRE(src.CalculateAttenuation(10.0f) == 0.0f);
    
    src.DistanceModel = AudioDistanceModel::InverseDistance;
    REQUIRE(src.CalculateAttenuation(1.0f) == 1.0f);
    REQUIRE(src.CalculateAttenuation(5.0f) < 1.0f);
    REQUIRE(src.CalculateAttenuation(5.0f) > 0.0f);
    
    src.DistanceModel = AudioDistanceModel::ExponentialDistance;
    REQUIRE(src.CalculateAttenuation(1.0f) == 1.0f);
    REQUIRE(src.CalculateAttenuation(10.0f) == 0.1f);
}

TEST_CASE("Audio - Distance Model Rolloff Effect", "[audio]")
{
    AudioSourceComponent src;
    src.ReferenceDistance = 1.0f;
    src.MaxDistance = 10.0f;
    src.DistanceModel = AudioDistanceModel::Linear;
    
    src.RolloffFactor = 1.0f;
    float atten1 = src.CalculateAttenuation(5.5f);
    
    src.RolloffFactor = 0.5f;
    float atten2 = src.CalculateAttenuation(5.5f);
    
    REQUIRE(atten2 > atten1);
}

TEST_CASE("Audio - AudioSourceComponent Default Values", "[audio]")
{
    AudioSourceComponent src;
    
    REQUIRE(src.MinDistance == 1.0f);
    REQUIRE(src.MaxDistance == 100.0f);
    REQUIRE(src.RolloffFactor == 1.0f);
    REQUIRE(src.ReferenceDistance == 1.0f);
    REQUIRE(src.DistanceModel == AudioDistanceModel::InverseDistance);
    REQUIRE(src.EnableObstruction == true);
    REQUIRE(src.ObstructionMultiplier == 1.0f);
    REQUIRE(src.EnableCone == false);
    REQUIRE(src.InnerConeAngle == 360.0f);
    REQUIRE(src.OuterConeAngle == 360.0f);
    REQUIRE(src.OuterConeVolume == 0.0f);
}

TEST_CASE("Audio - AudioListenerComponent Default Values", "[audio]")
{
    AudioListenerComponent listener;
    
    REQUIRE(listener.IsActive == true);
    REQUIRE(listener.EnableObstruction == true);
    REQUIRE(listener.MaxObstructionDistance == 100.0f);
    REQUIRE(listener.ObstructionRays == 8);
    REQUIRE(listener.EnableLowpassOnObstruction == true);
    REQUIRE(listener.LowpassMinFrequency == 800.0f);
    REQUIRE(listener.LowpassMaxFrequency == 22050.0f);
    REQUIRE(listener.ObstructionTransitionTime == 0.1f);
}

TEST_CASE("Audio - Obstacle Ray Structure", "[audio]")
{
    AudioObstructionRay ray;
    ray.Direction = Math::Vec3f(1.0f, 0.0f, 0.0f);
    ray.Distance = 10.0f;
    ray.ObstructionAmount = 0.5f;
    
    REQUIRE(ray.Direction.x == 1.0f);
    REQUIRE(ray.Distance == 10.0f);
    REQUIRE(ray.ObstructionAmount == 0.5f);
}

TEST_CASE("Audio - Distance Attenuation Boundary Cases", "[audio]")
{
    AudioSourceComponent src;
    src.ReferenceDistance = 5.0f;
    src.MaxDistance = 20.0f;
    src.DistanceModel = AudioDistanceModel::Linear;
    src.RolloffFactor = 1.0f;
    
    REQUIRE(src.CalculateAttenuation(0.0f) == 1.0f);
    REQUIRE(src.CalculateAttenuation(3.0f) == 1.0f);
    REQUIRE(src.CalculateAttenuation(5.0f) == 1.0f);
    REQUIRE(src.CalculateAttenuation(20.0f) == 0.0f);
    REQUIRE(src.CalculateAttenuation(100.0f) == 0.0f);
}

TEST_CASE("Audio - Distance Attenuation Monotonic", "[audio]")
{
    AudioSourceComponent src;
    src.ReferenceDistance = 1.0f;
    src.MaxDistance = 10.0f;
    src.DistanceModel = AudioDistanceModel::InverseDistance;
    src.RolloffFactor = 1.0f;
    
    float prev = 1.0f;
    for (float d = 1.0f; d <= 10.0f; d += 0.5f) {
        float curr = src.CalculateAttenuation(d);
        REQUIRE(curr <= prev);
        prev = curr;
    }
    REQUIRE(prev >= 0.0f);
    REQUIRE(prev <= 1.0f);
}

TEST_CASE("Audio - Cone Angle Calculations", "[audio]")
{
    auto normalize = [](Math::Vec3f v) {
        float len = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
        if (len > 0.0001f) return Math::Vec3f(v.x/len, v.y/len, v.z/len);
        return v;
    };
    
    auto dot = [](Math::Vec3f a, Math::Vec3f b) {
        return a.x*b.x + a.y*b.y + a.z*b.z;
    };
    
    auto angleBetweenVectors = [&](const Math::Vec3f& a, const Math::Vec3f& b) -> float {
        Math::Vec3f na = normalize(a);
        Math::Vec3f nb = normalize(b);
        float d = dot(na, nb);
        d = std::max(-1.0f, std::min(1.0f, d));
        return std::acos(d) * 180.0f / 3.14159f;
    };
    
    Math::Vec3f forward(0.0f, 0.0f, 1.0f);
    Math::Vec3f same(0.0f, 0.0f, 1.0f);
    Math::Vec3f behind(0.0f, 0.0f, -1.0f);
    Math::Vec3f side(1.0f, 0.0f, 0.0f);
    
    REQUIRE(angleBetweenVectors(forward, same) < 1.0f);
    REQUIRE(angleBetweenVectors(forward, behind) > 90.0f);
    REQUIRE(angleBetweenVectors(forward, side) < 90.0f);
}

TEST_CASE("Audio - Obstruction Transition Calculation", "[audio]")
{
    auto calculateLowpassFrequency = [](float obstruction, float minFreq, float maxFreq) -> float {
        return maxFreq - (obstruction * (maxFreq - minFreq));
    };
    
    REQUIRE(calculateLowpassFrequency(0.0f, 800.0f, 22050.0f) == 22050.0f);
    REQUIRE(calculateLowpassFrequency(0.5f, 800.0f, 22050.0f) == 11425.0f);
    REQUIRE(calculateLowpassFrequency(1.0f, 800.0f, 22050.0f) == 800.0f);
}

TEST_CASE("Audio - Multi-Ray Obstruction Average", "[audio]")
{
    std::vector<AudioObstructionRay> rays = {
        {Math::Vec3f(1.0f, 0.0f, 0.0f), 10.0f, 0.0f},
        {Math::Vec3f(0.707f, 0.707f, 0.0f), 10.0f, 0.5f},
        {Math::Vec3f(0.0f, 1.0f, 0.0f), 10.0f, 1.0f}
    };
    
    float total = 0.0f;
    for (const auto& ray : rays) {
        total += ray.ObstructionAmount;
    }
    float average = total / rays.size();
    
    REQUIRE(average > 0.4f);
    REQUIRE(average < 0.6f);
}

TEST_CASE("Audio - Distance Clamping", "[audio]")
{
    AudioSourceComponent src;
    src.ReferenceDistance = 2.0f;
    src.MaxDistance = 8.0f;
    src.DistanceModel = AudioDistanceModel::Linear;
    src.RolloffFactor = 1.0f;
    
    REQUIRE(src.CalculateAttenuation(-5.0f) == 1.0f);
    REQUIRE(src.CalculateAttenuation(0.0f) == 1.0f);
    REQUIRE(src.CalculateAttenuation(2.0f) == 1.0f);
    REQUIRE(src.CalculateAttenuation(5.0f) < 1.0f);
    REQUIRE(src.CalculateAttenuation(5.0f) > 0.0f);
    REQUIRE(src.CalculateAttenuation(8.0f) == 0.0f);
    REQUIRE(src.CalculateAttenuation(100.0f) == 0.0f);
}

TEST_CASE("Audio - Category Enum Values", "[audio]")
{
    REQUIRE(static_cast<int>(AudioCategory::Master) >= 0);
    REQUIRE(static_cast<int>(AudioCategory::SFX) >= 0);
    REQUIRE(static_cast<int>(AudioCategory::Music) >= 0);
    REQUIRE(static_cast<int>(AudioCategory::Voice) >= 0);
    REQUIRE(static_cast<int>(AudioCategory::Ambient) >= 0);
    REQUIRE(static_cast<int>(AudioCategory::UI) >= 0);
}

TEST_CASE("Audio - Category String Conversion", "[audio]")
{
    REQUIRE(strcmp(CategoryToString(AudioCategory::Master), "Master") == 0);
    REQUIRE(strcmp(CategoryToString(AudioCategory::SFX), "SFX") == 0);
    REQUIRE(strcmp(CategoryToString(AudioCategory::Music), "Music") == 0);
    REQUIRE(strcmp(CategoryToString(AudioCategory::Voice), "Voice") == 0);
    REQUIRE(strcmp(CategoryToString(AudioCategory::Ambient), "Ambient") == 0);
    REQUIRE(strcmp(CategoryToString(AudioCategory::UI), "UI") == 0);
}

TEST_CASE("Audio - Category From String", "[audio]")
{
    REQUIRE(CategoryFromString("master") == AudioCategory::Master);
    REQUIRE(CategoryFromString("SFX") == AudioCategory::SFX);
    REQUIRE(CategoryFromString("music") == AudioCategory::Music);
    REQUIRE(CategoryFromString("voice") == AudioCategory::Voice);
    REQUIRE(CategoryFromString("Ambient") == AudioCategory::Ambient);
    REQUIRE(CategoryFromString("UI") == AudioCategory::UI);
    REQUIRE(CategoryFromString("unknown") == AudioCategory::SFX);
}

TEST_CASE("Audio - AudioSourceComponent Category Default", "[audio]")
{
    AudioSourceComponent src;
    REQUIRE(src.Category == AudioCategory::SFX);
    src.Category = AudioCategory::Music;
    REQUIRE(src.Category == AudioCategory::Music);
}

TEST_CASE("Audio - Ducking Rule Structure", "[audio]")
{
    DuckingRule rule;
    rule.triggerCategory = AudioCategory::SFX;
    rule.targetCategory = AudioCategory::Music;
    rule.duckAmount = 0.3f;
    rule.fadeInTime = 0.1f;
    rule.fadeOutTime = 0.5f;
    rule.holdTime = 0.3f;
    rule.enabled = true;
    
    REQUIRE(rule.triggerCategory == AudioCategory::SFX);
    REQUIRE(rule.targetCategory == AudioCategory::Music);
    REQUIRE(rule.duckAmount == 0.3f);
    REQUIRE(rule.enabled == true);
}

TEST_CASE("Audio - Ducking Rule Constructor", "[audio]")
{
    DuckingRule rule(AudioCategory::Voice, AudioCategory::Music, 0.5f);
    
    REQUIRE(rule.triggerCategory == AudioCategory::Voice);
    REQUIRE(rule.targetCategory == AudioCategory::Music);
    REQUIRE(rule.duckAmount == 0.5f);
    REQUIRE(rule.enabled == true);
}
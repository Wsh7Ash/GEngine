#include "../catch_amalgamated.hpp"

#include "../../src/core/ecs/components/AudioSourceComponent.h"
#include "../../src/core/ecs/components/AudioListenerComponent.h"
#include <cmath>

using namespace ge::ecs;

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
    auto angleBetweenVectors = [](const Math::Vec3f& a, const Math::Vec3f& b) -> float {
        float dot = Math::Vec3f::Dot(a.Normalized(), b.Normalized());
        dot = std::max(-1.0f, std::min(1.0f, dot));
        return std::acos(dot) * 180.0f / 3.14159f;
    };
    
    Math::Vec3f forward = Math::Vec3f::Forward();
    Math::Vec3f same = Math::Vec3f::Forward();
    Math::Vec3f behind = Math::Vec3f::Back();
    Math::Vec3f side = Math::Vec3f::Right();
    
    REQUIRE(angleBetweenVectors(forward, same) == 0.0f);
    REQUIRE(angleBetweenVectors(forward, behind) > 90.0f);
    REQUIRE(angleBetweenVectors(forward, side) < 90.0f);
    REQUIRE(angleBetweenVectors(forward, side) > 80.0f);
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
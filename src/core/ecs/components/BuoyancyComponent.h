#pragma once

#include "../../math/VecTypes.h"
#include "../../math/quaternion.h"

namespace ge {
namespace ecs {

enum class BuoyancyMode {
    Simple,
    Volumetric,
    Linear
};

struct BuoyancyComponent {
    BuoyancyMode Mode = BuoyancyMode::Volumetric;
    
    float FluidDensity = 1000.0f;
    
    float LinearDrag = 1.0f;
    float AngularDrag = 1.0f;
    
    float FlowStrength = 0.0f;
    Math::Vec3f FlowDirection = {0.0f, 0.0f, 1.0f};
    
    float WaveHeight = 0.0f;
    float WaveFrequency = 1.0f;
    float WaveSpeed = 1.0f;
    
    int SamplePoints = 4;
    
    Math::Vec3f Offset = {0.0f, 0.0f, 0.0f};
    
    float SubmergedAmount = 0.0f;
    float BuoyancyForce = 0.0f;
    
    bool IsSubmerged = false;
};

struct WaterVolumeComponent {
    Math::Vec3f Position = {0.0f, 0.0f, 0.0f};
    Math::Vec3f Size = {100.0f, 10.0f, 100.0f};
    
    float WaterHeight = 0.0f;
    
    float Density = 1000.0f;
    float Viscosity = 0.001f;
    
    Math::Vec3f FlowDirection = {0.0f, 0.0f, 0.0f};
    float FlowSpeed = 0.0f;
    
    float WaveAmplitude = 0.5f;
    float WaveFrequency = 0.5f;
    float WaveSpeed = 1.0f;
    
    Math::Vec3f WaterColor = {0.0f, 0.4f, 0.6f};
    float Transparency = 0.7f;
    
    bool IsActive = true;
    
    std::string NormalMapPath = "";
    std::string CausticsPath = "";
};

} // namespace ecs
} // namespace ge

// ================================================================
//  StressTest.h
//  Scene stress test for Domain 4 Assessment.
//  Load and profile a scene with 10k entities and C# components.
// ================================================================

#pragma once

#include "../core/ecs/World.h"
#include "../core/ecs/components/TransformComponent.h"
#include "../core/ecs/components/MeshComponent.h"
#include "../core/ecs/components/LightComponent.h"
#include "../core/scripting/ManagedScriptComponent.h"
#include "../core/debug/ProfilerEx.h"
#include <vector>
#include <random>
#include <chrono>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

namespace ge {
namespace tests {

enum class StressTestComponentType {
    Transform = 1 << 0,
    Mesh = 1 << 1,
    Light = 1 << 2,
    Sprite = 1 << 3,
    RigidBody2D = 1 << 4,
    ManagedScript = 1 << 5,
    All = 0xFFFFFFFF
};

inline StressTestComponentType operator|(StressTestComponentType a, StressTestComponentType b) {
    return static_cast<StressTestComponentType>(static_cast<int>(a) | static_cast<int>(b));
}

inline StressTestComponentType operator&(StressTestComponentType a, StressTestComponentType b) {
    return static_cast<StressTestComponentType>(static_cast<int>(a) & static_cast<int>(b));
}

struct StressTestConfig {
    int entityCount = 10000;
    int threadCount = 4;
    StressTestComponentType componentTypes = StressTestComponentType::Transform | StressTestComponentType::ManagedScript;
    float spawnRadius = 100.0f;
    int batchSize = 100;
    bool enableRandomRotation = true;
    bool enableRandomScale = true;
    int scriptTypeCount = 5;
    std::string scriptAssembly;
};

struct StressTestResults {
    double entityCreationTimeMs = 0.0;
    double entityDestructionTimeMs = 0.0;
    double firstFrameTimeMs = 0.0;
    double avgFrameTimeMs = 0.0;
    double minFrameTimeMs = 0.0;
    double maxFrameTimeMs = 0.0;
    int totalEntities = 0;
    size_t peakMemoryBytes = 0;
    int entitiesWithScripts = 0;
    int entitiesWithTransforms = 0;
    int entitiesWithMeshes = 0;
    int entitiesWithLights = 0;
    double systemInitTimeMs = 0.0;
    std::vector<std::pair<std::string, double>> systemTimes;
    bool threadSafe = true;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

class StressTest {
public:
    StressTest();
    ~StressTest();

    void SetConfig(const StressTestConfig& config);
    const StressTestConfig& GetConfig() const { return config_; }

    void SetWorld(ecs::World* world);
    ecs::World* GetWorld() const { return world_; }

    StressTestResults Execute();

    void CreateEntities();
    void DestroyEntities();

    void Update(float dt);

    int GetEntityCount() const { return entityCount_; }
    std::vector<ecs::Entity> GetEntities() const { return entities_; }

    bool IsRunning() const { return isRunning_; }
    void SetRunning(bool running) { isRunning_ = running; }

    static std::unique_ptr<StressTest> CreateDefault();

private:
    void CreateEntityBatch(int start, int count);
    ecs::Entity CreateSingleEntity(int index);
    void SetupComponentDistribution();

    ecs::World* world_ = nullptr;
    StressTestConfig config_;
    std::vector<ecs::Entity> entities_;
    int entityCount_ = 0;
    bool isRunning_ = false;
    bool entitiesCreated_ = false;

    std::mt19937 rng_;
    std::uniform_real_distribution<float> posDist_;
    std::uniform_real_distribution<float> scaleDist_;
    std::uniform_real_distribution<float> rotDist_;
};

class StressTestRunner {
public:
    StressTestRunner();
    ~StressTestRunner();

    void SetConfig(const StressTestConfig& config);
    StressTestConfig& GetConfig() { return config_; }

    void SetProfiler(debug::ProfilerEx* profiler);
    debug::ProfilerEx* GetProfiler() const { return profiler_; }

    StressTestResults Run();

    void SetPreUpdateCallback(std::function<void(float)> callback);
    void SetPostUpdateCallback(std::function<void(float)> callback);

    void SetValidationEnabled(bool enabled);
    bool IsValidationEnabled() const { return validate_; }

    void SetWarmupFrames(int frames);
    int GetWarmupFrames() const { return warmupFrames_; }

    void SetBenchmarkFrames(int frames);
    int GetBenchmarkFrames() const { return benchmarkFrames_; }

private:
    void Warmup();
    void Benchmark();
    void ValidateResults(StressTestResults& results);

    StressTestConfig config_;
    StressTest* test_ = nullptr;
    debug::ProfilerEx* profiler_ = nullptr;

    bool validate_ = true;
    int warmupFrames_ = 60;
    int benchmarkFrames_ = 300;

    std::function<void(float)> preUpdateCallback_;
    std::function<void(float)> postUpdateCallback_;

    std::vector<double> frameTimes_;
};

class SceneProfiler {
public:
    SceneProfiler();
    ~SceneProfiler();

    void Initialize(ecs::World* world);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void BeginSystem(const std::string& systemName);
    void EndSystem(const std::string& systemName);

    struct SystemMetrics {
        std::string name;
        double totalTimeMs = 0.0;
        double avgTimeMs = 0.0;
        double minTimeMs = 0.0;
        double maxTimeMs = 0.0;
        int callCount = 0;
    };

    const std::vector<SystemMetrics>& GetSystemMetrics() const { return systemMetrics_; }
    const SystemMetrics* GetSystemMetrics(const std::string& name) const;

    void Reset();

    void SetOutputFile(const std::string& filepath);
    void ExportResults();

private:
    ecs::World* world_ = nullptr;
    std::chrono::high_resolution_clock::time_point frameStart_;
    std::chrono::high_resolution_clock::time_point systemStart_;

    std::string currentSystem_;
    std::vector<SystemMetrics> systemMetrics_;
    std::unordered_map<std::string, size_t> systemIndex_;

    std::string outputFile_;
    bool initialized_ = false;
};

} // namespace tests
} // namespace ge

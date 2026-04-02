#pragma once

#include "../ecs/System.h"
#include "SceneManager.h"
#include <memory>
#include <vector>
#include <atomic>

namespace ge {
namespace ecs {
class World;
}
namespace scene {

class SceneStreamingSystem : public ecs::System {
public:
    explicit SceneStreamingSystem(ecs::World* world);
    virtual ~SceneStreamingSystem() = default;
    
    void Update(ecs::World& world, float deltaTime);
    
    void SetActiveEntity(ecs::Entity entity);
    ecs::Entity GetActiveEntity() const { return focusEntity_; }
    
    void SetStreamingEnabled(bool enabled);
    bool IsStreamingEnabled() const;
    
    void SetStreamingRadius(float radius);
    float GetStreamingRadius() const;
    
    void ForceLoadChunk(uint32_t chunkId);
    void ForceUnloadChunk(uint32_t chunkId);
    
    const LoadingProgress& GetLoadingProgress() const;
    
    void OnEntityMoved(ecs::Entity entity, const Math::Vec3f& newPosition);
    
    void DebugDrawLoadedChunks() const;
    
private:
    void UpdateFocusPoint();
    void CheckChunkLoading();
    void CheckChunkUnloading();
    
    ecs::World* world_ = nullptr;
    SceneManager* sceneManager_ = nullptr;
    
    ecs::Entity focusEntity_ = ecs::INVALID_ENTITY;
    Math::Vec3f focusPosition_ = {0, 0, 0};
    
    float updateTimer_ = 0.0f;
    float updateInterval_ = 0.25f;
    
    bool isEnabled_ = true;
    bool wasEnabled_ = true;
    
    std::vector<ecs::Entity> movingEntities_;
    
    std::atomic<int> loadedChunkCount_ = 0;
    std::atomic<int> pendingChunkCount_ = 0;
};

}
}

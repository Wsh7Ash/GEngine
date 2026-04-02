#include "SceneStreamingSystem.h"
#include "debug/log.h"

namespace ge {
namespace scene {

SceneStreamingSystem::SceneStreamingSystem(ecs::World* world)
    : world_(world) {
    sceneManager_ = &SceneManager::Get();
}

void SceneStreamingSystem::Update(ecs::World& world, float deltaTime) {
    if (!isEnabled_) {
        return;
    }
    
    updateTimer_ += deltaTime;
    if (updateTimer_ < updateInterval_) {
        return;
    }
    updateTimer_ = 0.0f;
    
    UpdateFocusPoint();
    CheckChunkLoading();
    CheckChunkUnloading();
}

void SceneStreamingSystem::UpdateFocusPoint() {
    if (focusEntity_ != ecs::INVALID_ENTITY && world_->IsAlive(focusEntity_)) {
        if (world_->HasComponent<TransformComponent>(focusEntity_)) {
            auto& transform = world_->GetComponent<TransformComponent>(focusEntity_);
            focusPosition_ = transform.Translation;
        }
    }
    
    if (sceneManager_) {
        sceneManager_->SetFocusPoint(focusPosition_);
        sceneManager_->UpdateStreaming(focusPosition_);
    }
}

void SceneStreamingSystem::CheckChunkLoading() {
    if (!sceneManager_ || !sceneManager_->IsStreamingEnabled()) {
        return;
    }
    
    auto& progress = sceneManager_->GetLoadingProgress();
    if (progress.isLoading) {
        pendingChunkCount_ = static_cast<int>(progress.totalChunks - progress.loadedChunks);
    }
}

void SceneStreamingSystem::CheckChunkUnloading() {
    if (!sceneManager_) {
        return;
    }
}

void SceneStreamingSystem::SetActiveEntity(ecs::Entity entity) {
    focusEntity_ = entity;
}

void SceneStreamingSystem::SetStreamingEnabled(bool enabled) {
    isEnabled_ = enabled;
    if (sceneManager_) {
        sceneManager_->SetStreamingEnabled(enabled);
    }
}

bool SceneStreamingSystem::IsStreamingEnabled() const {
    return isEnabled_ && sceneManager_ && sceneManager_->IsStreamingEnabled();
}

void SceneStreamingSystem::SetStreamingRadius(float radius) {
    if (sceneManager_) {
        sceneManager_->SetStreamingRadius(radius);
    }
}

float SceneStreamingSystem::GetStreamingRadius() const {
    if (sceneManager_) {
        return sceneManager_->GetStreamingRadius();
    }
    return 0.0f;
}

void SceneStreamingSystem::ForceLoadChunk(uint32_t chunkId) {
    if (sceneManager_) {
        sceneManager_->LoadChunk(chunkId);
    }
}

void SceneStreamingSystem::ForceUnloadChunk(uint32_t chunkId) {
    if (sceneManager_) {
        sceneManager_->UnloadChunk(chunkId);
    }
}

const LoadingProgress& SceneStreamingSystem::GetLoadingProgress() const {
    static LoadingProgress empty;
    if (sceneManager_) {
        return sceneManager_->GetLoadingProgress();
    }
    return empty;
}

void SceneStreamingSystem::OnEntityMoved(ecs::Entity entity, const Math::Vec3f& newPosition) {
    for (auto& existing : movingEntities_) {
        if (existing == entity) {
            return;
        }
    }
    movingEntities_.push_back(entity);
}

void SceneStreamingSystem::DebugDrawLoadedChunks() const {
    // Would use renderer to draw chunk bounds
    // For now this is a placeholder
}

}
}

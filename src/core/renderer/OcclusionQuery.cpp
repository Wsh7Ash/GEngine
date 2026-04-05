#include "OcclusionQuery.h"
#include "../debug/log.h"
#include "../ecs/components/MeshComponent.h"
#include "../ecs/components/TransformComponent.h"
#include <glad/glad.h>
#include <algorithm>

namespace ge {
namespace renderer {

OcclusionQuery& OcclusionQuery::Get() {
    static OcclusionQuery instance;
    return instance;
}

void OcclusionQuery::Initialize(uint32_t maxQueries) {
    if (initialized_) return;

    queryPool_.resize(maxQueries);
    freeQueryIndices_.reserve(maxQueries);

    for (uint32_t i = 0; i < maxQueries; ++i) {
        glGenQueries(1, &queryPool_[i].glQuery);
        freeQueryIndices_.push_back(i);
    }

    initialized_ = true;
    GE_LOG_INFO("OcclusionQuery initialized with %u queries", maxQueries);
}

void OcclusionQuery::Shutdown() {
    if (!initialized_) return;

    for (auto& q : queryPool_) {
        if (q.glQuery) {
            glDeleteQueries(1, &q.glQuery);
        }
    }

    queryPool_.clear();
    freeQueryIndices_.clear();
    entityToQueryMap_.clear();
    initialized_ = false;
}

void OcclusionQuery::BeginFrame() {
    currentFrame_++;
    currentQueryIndex_ = 0;
}

void OcclusionQuery::EndFrame() {
    for (auto& q : queryPool_) {
        q.resultReady = false;
    }
}

uint32_t OcclusionQuery::IssueQuery() {
    if (!enabled_ || !useGPUQueries_ || freeQueryIndices_.empty()) {
        return UINT32_MAX;
    }

    uint32_t index = freeQueryIndices_.back();
    freeQueryIndices_.pop_back();

    queryPool_[index].frameIssued = currentFrame_;
    queryPool_[index].resultReady = false;

    glBeginQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE, queryPool_[index].glQuery);

    return index;
}

bool OcclusionQuery::GetQueryResult(uint32_t queryId, bool& visible) {
    if (queryId == UINT32_MAX || !enabled_) {
        visible = true;
        return true;
    }

    if (!useGPUQueries_) {
        visible = true;
        return true;
    }

    auto& q = queryPool_[queryId];

    if (q.resultReady) {
        visible = q.wasVisible;
        return true;
    }

    GLuint result = GL_FALSE;
    glGetQueryObjectuiv(q.glQuery, GL_QUERY_RESULT_AVAILABLE, &result);

    if (result) {
        glGetQueryObjectuiv(q.glQuery, GL_QUERY_RESULT, &result);
        q.wasVisible = (result != GL_FALSE);
        q.resultReady = true;

        freeQueryIndices_.push_back(queryId);
        visible = q.wasVisible;
        return true;
    }

    visible = true;
    return false;
}

bool OcclusionQuery::IsQueryReady(uint32_t queryId) {
    if (queryId == UINT32_MAX || !enabled_) return true;

    auto& q = queryPool_[queryId];
    if (q.resultReady) return true;

    GLuint result = GL_FALSE;
    glGetQueryObjectuiv(q.glQuery, GL_QUERY_RESULT_AVAILABLE, &result);
    return (result != GL_FALSE);
}

void OcclusionQuery::BindEntityQuery(uint32_t queryId, uint64_t entityKey) {
    if (queryId == UINT32_MAX || !enabled_) return;

    queryPool_[queryId].entityKey = entityKey;
    entityToQueryMap_[entityKey] = queryId;
}

uint64_t OcclusionQuery::GetEntityFromQuery(uint32_t queryId) const {
    if (queryId == UINT32_MAX || queryId >= queryPool_.size()) return 0;
    return queryPool_[queryId].entityKey;
}

void CPUCuller::CullFrustum(
    const std::vector<uint64_t>& entities,
    const std::unordered_map<uint64_t, MeshComponent*>& meshes,
    const std::unordered_map<uint64_t, TransformComponent*>& transforms,
    CullResult& result) {

    if (!camera_) return;

    Math::Frustum frustum;
    frustum.FromMatrix(camera_->GetViewProjectionMatrix());

    for (uint64_t entity : entities) {
        auto meshIt = meshes.find(entity);
        auto transformIt = transforms.find(entity);

        if (meshIt == meshes.end() || transformIt == transforms.end()) continue;

        auto* meshComp = meshIt->second;
        auto* transform = transformIt->second;

        if (!meshComp || !meshComp->MeshPtr || !meshComp->IsVisible) continue;

        Math::Mat4f model = Math::Mat4f::Translate(transform->position) *
                           transform->rotation.ToMat4x4() *
                           Math::Mat4f::Scale(transform->scale);

        Math::AABB worldAABB = meshComp->MeshPtr->GetAABB().Transform(model);

        if (frustum.Intersect(worldAABB)) {
            result.visibleEntities.push_back(entity);
        } else {
            result.culledCount++;
        }
    }
}

void CPUCuller::CullDistance(
    const std::vector<uint64_t>& entities,
    const std::unordered_map<uint64_t, MeshComponent*>& meshes,
    const std::unordered_map<uint64_t, TransformComponent*>& transforms,
    CullResult& result) {

    if (!camera_) return;

    const Math::Vec3f& camPos = camera_->GetPosition();

    for (uint64_t entity : entities) {
        auto meshIt = meshes.find(entity);
        auto transformIt = transforms.find(entity);

        if (meshIt == meshes.end() || transformIt == meshes.end()) continue;

        auto* meshComp = meshIt->second;
        auto* transform = transformIt->second;

        if (!meshComp || !meshComp->IsVisible) continue;

        float dist = (transform->position - camPos).Length();

        if (dist >= meshComp->MinDrawDistance && dist <= meshComp->MaxDrawDistance) {
            result.visibleEntities.push_back(entity);
        } else {
            result.culledCount++;
        }
    }
}

void CPUCuller::FrustumAndDistanceCull(
    const std::vector<uint64_t>& entities,
    const std::unordered_map<uint64_t, MeshComponent*>& meshes,
    const std::unordered_map<uint64_t, TransformComponent*>& transforms,
    CullResult& result) {

    if (!camera_) {
        result.visibleEntities = entities;
        return;
    }

    Math::Frustum frustum;
    frustum.FromMatrix(camera_->GetViewProjectionMatrix());

    const Math::Vec3f& camPos = camera_->GetPosition();

    for (uint64_t entity : entities) {
        auto meshIt = meshes.find(entity);
        auto transformIt = transforms.find(entity);

        if (meshIt == meshes.end() || transformIt == transforms.end()) continue;

        auto* meshComp = meshIt->second;
        auto* transform = transformIt->second;

        if (!meshComp || !meshComp->MeshPtr || !meshComp->IsVisible) continue;

        Math::Mat4f model = Math::Mat4f::Translate(transform->position) *
                           transform->rotation.ToMat4x4() *
                           Math::Mat4f::Scale(transform->scale);

        Math::AABB worldAABB = meshComp->MeshPtr->GetAABB().Transform(model);

        if (!frustum.Intersect(worldAABB)) {
            result.culledCount++;
            continue;
        }

        float dist = (transform->position - camPos).Length();
        if (dist < meshComp->MinDrawDistance || dist > meshComp->MaxDrawDistance) {
            result.culledCount++;
            continue;
        }

        result.visibleEntities.push_back(entity);
    }
}

} // namespace renderer
} // namespace ge

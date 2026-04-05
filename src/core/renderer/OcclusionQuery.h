#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>

#include "../math/BoundingVolumes.h"

namespace ge {
namespace renderer {

class PerspectiveCamera;
class MeshComponent;
struct TransformComponent;

class OcclusionQuery {
public:
    static OcclusionQuery& Get();

    void Initialize(uint32_t maxQueries = 512);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    uint32_t IssueQuery();
    bool GetQueryResult(uint32_t queryId, bool& visible);
    bool IsQueryReady(uint32_t queryId);

    void BindEntityQuery(uint32_t queryId, uint64_t entityKey);
    uint64_t GetEntityFromQuery(uint32_t queryId) const;

    bool IsEnabled() const { return enabled_; }
    void SetEnabled(bool enabled) { enabled_ = enabled; }

    bool IsGPUBased() const { return useGPUQueries_; }
    void SetGPUBased(bool gpu) { useGPUQueries_ = gpu; }

private:
    OcclusionQuery() = default;
    ~OcclusionQuery() = default;
    OcclusionQuery(const OcclusionQuery&) = delete;
    OcclusionQuery& operator=(const OcclusionQuery&) = delete;

    struct QueryData {
        uint32_t glQuery = 0;
        uint64_t entityKey = 0;
        bool resultReady = false;
        bool wasVisible = false;
        int frameIssued = -1;
    };

    std::vector<QueryData> queryPool_;
    std::unordered_map<uint64_t, uint32_t> entityToQueryMap_;
    std::vector<uint32_t> freeQueryIndices_;
    uint32_t currentQueryIndex_ = 0;
    uint32_t currentFrame_ = 0;
    bool enabled_ = false;
    bool useGPUQueries_ = false;
    bool initialized_ = false;
};

class CPUCuller {
public:
    struct CullResult {
        std::vector<uint64_t> visibleEntities;
        uint32_t culledCount = 0;
    };

    void SetCamera(const PerspectiveCamera* camera) { camera_ = camera; }

    void CullFrustum(const std::vector<uint64_t>& entities,
                     const std::unordered_map<uint64_t, MeshComponent*>& meshes,
                     const std::unordered_map<uint64_t, TransformComponent*>& transforms,
                     CullResult& result);

    void CullDistance(const std::vector<uint64_t>& entities,
                      const std::unordered_map<uint64_t, MeshComponent*>& meshes,
                      const std::unordered_map<uint64_t, TransformComponent*>& transforms,
                      CullResult& result);

    void FrustumAndDistanceCull(
        const std::vector<uint64_t>& entities,
        const std::unordered_map<uint64_t, MeshComponent*>& meshes,
        const std::unordered_map<uint64_t, TransformComponent*>& transforms,
        CullResult& result);

private:
    const PerspectiveCamera* camera_ = nullptr;
};

} // namespace renderer
} // namespace ge

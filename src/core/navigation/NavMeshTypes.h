#pragma once

#include "../math/VecTypes.h"
#include "../math/quaternion.h"
#include <vector>
#include <memory>
#include <cstdint>

namespace ge {
namespace navigation {

struct dtNavMesh;
struct dtNavMeshQuery;
struct dtCrowd;
struct dtQueryFilter;
struct dtPoly;
struct dtTileRef;

enum class NavMeshQueryResult {
    Success = 0,
    Failure = 1,
    OutOfNodes = 2,
    InvalidParam = 3,
    BufferTooSmall = 4
};

enum class PathFindingMode {
    Straight = 0,
    Smoother = 1
};

struct NavMeshConfig {
    float cellSize = 0.3f;
    float cellHeight = 0.2f;
    float agentHeight = 2.0f;
    float agentRadius = 0.6f;
    float agentMaxClimb = 1.0f;
    float agentMaxSlope = 45.0f;
    float regionMinSize = 8.0f;
    float regionMergedSize = 20.0f;
    float edgeMaxLength = 12.0f;
    float edgeMaxError = 1.3f;
    float detailSampleDist = 6.0f;
    float detailSampleMaxError = 1.0f;
    int tileWidth = 16;
    int tileHeight = 16;
    float navMeshBMin[3] = {0, 0, 0};
    float navMeshBMax[3] = {0, 0, 0};
};

struct PathPoint {
    Math::Vec3f position;
    Math::Vec3f velocity;
    uint32_t flags = 0;
    float cornerRadius = 0.0f;
    int referenceTile = 0;
    
    PathPoint() = default;
    PathPoint(const Math::Vec3f& pos) : position(pos) {}
};

struct PathData {
    std::vector<PathPoint> waypoints;
    std::vector<Math::Vec3f> corners;
    float pathLength = 0.0f;
    bool isPartial = false;
    uint32_t pathFlags = 0;
    
    bool IsValid() const { return !waypoints.empty(); }
    size_t GetWaypointCount() const { return waypoints.size(); }
    void Clear() { 
        waypoints.clear(); 
        corners.clear(); 
        pathLength = 0.0f; 
        isPartial = false;
    }
};

struct AgentDebugData {
    int agentIndex = -1;
    Math::Vec3f position;
    Math::Vec3f velocity;
    Math::Vec3f desiredVelocity;
    Math::Vec3f target;
    float distanceToTarget = 0.0f;
    bool isOnNavMesh = false;
    bool isOnObstacle = false;
};

class NavMesh {
public:
    NavMesh();
    ~NavMesh();
    
    bool IsValid() const { return navMesh_ != nullptr; }
    dtNavMesh* Get() { return navMesh_; }
    const dtNavMesh* Get() const { return navMesh_; }
    
    void SetBoundingBox(const Math::Vec3f& min, const Math::Vec3f& max);
    Math::Vec3f GetBoundingBoxMin() const { return bMin_; }
    Math::Vec3f GetBoundingBoxMax() const { return bMax_; }
    
    int GetTileCount() const;
    int GetPolyCount() const;
    
private:
    dtNavMesh* navMesh_ = nullptr;
    Math::Vec3f bMin_;
    Math::Vec3f bMax_;
    
    friend class NavMeshGenerator;
    friend class NavMeshQuery;
    friend class NavMeshCrowd;
    
    void SetNavMesh(dtNavMesh* navMesh);
};

class NavMeshQuery {
public:
    NavMeshQuery();
    ~NavMeshQuery();
    
    bool IsValid() const { return query_ != nullptr; }
    dtNavMeshQuery* Get() { return query_; }
    const dtNavMeshQuery* Get() const { return query_; }
    
    void SetNavMesh(NavMesh* navMesh);
    void SetNavMesh(NavMesh& navMesh);
    
    NavMeshQueryResult FindPath(const Math::Vec3f& start, const Math::Vec3f& end, 
                                 PathData& path, PathFindingMode mode = PathFindingMode::Straight);
    
    NavMeshQueryResult GetClosestPoint(const Math::Vec3f& position, Math::Vec3f& closest);
    
    NavMeshQueryResult GetRandomPoint(Math::Vec3f& randomPoint);
    
    NavMeshQueryResult Raycast(const Math::Vec3f& start, const Math::Vec3f& end,
                               float& hitT, Math::Vec3f& hitNormal);
    
    NavMeshQueryResult GetPathDistance(const Math::Vec3f& start, const Math::Vec3f& end, 
                                       float& distance);
    
    NavMeshQueryResult GetPolysNearCircle(const Math::Vec3f& center, float radius,
                                           std::vector<uint64_t>& polys);
    
    NavMeshQueryResult GetPolysNearBox(const Math::Vec3f& center, const Math::Vec3f& halfExtents,
                                       const Math::Quatf& rotation, std::vector<uint64_t>& polys);
    
    void SetAreaCost(int area, float cost);
    void SetTileX(int x) { tileX_ = x; }
    void SetTileZ(int z) { tileZ_ = z; }
    
private:
    dtNavMeshQuery* query_ = nullptr;
    dtQueryFilter* filter_ = nullptr;
    NavMesh* navMesh_ = nullptr;
    int tileX_ = -1;
    int tileZ_ = -1;
    
    float areaCosts_[32];
};

class NavMeshCrowd {
public:
    NavMeshCrowd();
    ~NavMeshCrowd();
    
    bool IsValid() const { return crowd_ != nullptr; }
    dtCrowd* Get() { return crowd_; }
    const dtCrowd* Get() const { return crowd_; }
    
    void SetNavMesh(NavMesh* navMesh);
    void SetNavMesh(NavMesh& navMesh);
    
    int AddAgent(const Math::Vec3f& position, float radius, float height);
    void RemoveAgent(int agentIndex);
    
    void SetAgentTarget(int agentIndex, const Math::Vec3f& target);
    void SetAgentVelocity(int agentIndex, const Math::Vec3f& velocity);
    
    void GetAgentData(int agentIndex, AgentDebugData& data);
    
    Math::Vec3f GetAgentPosition(int agentIndex);
    Math::Vec3f GetAgentVelocity(int agentIndex);
    bool IsAgentActive(int agentIndex) const;
    
    void SetAgentMaxSpeed(int agentIndex, float maxSpeed);
    void SetAgentMaxAcceleration(int agentIndex, float maxAccel);
    void SetAgentRadius(int agentIndex, float radius);
    void SetAgentHeight(int agentIndex, float height);
    
    int GetAgentCount() const;
    int GetActiveAgentCount() const;
    
    void Update(float deltaTime, const std::vector<AgentDebugData>& externalAgents = {});
    
private:
    dtCrowd* crowd_ = nullptr;
    NavMesh* navMesh_ = nullptr;
};

}
}

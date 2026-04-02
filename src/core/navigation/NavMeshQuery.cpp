#include "NavMeshTypes.h"
#include "debug/log.h"

namespace ge {
namespace navigation {

NavMesh::NavMesh() {
    bMin_ = {0, 0, 0};
    bMax_ = {0, 0, 0};
}

NavMesh::~NavMesh() {
    if (navMesh_) {
        dtFreeNavMesh(navMesh_);
        navMesh_ = nullptr;
    }
}

void NavMesh::SetBoundingBox(const Math::Vec3f& min, const Math::Vec3f& max) {
    bMin_ = min;
    bMax_ = max;
    config_.navMeshBMin[0] = min.x;
    config_.navMeshBMin[1] = min.y;
    config_.navMeshBMin[2] = min.z;
    config_.navMeshBMax[0] = max.x;
    config_.navMeshBMax[1] = max.y;
    config_.navMeshBMax[2] = max.z;
}

int NavMesh::GetTileCount() const {
    if (!navMesh_) return 0;
    return navMesh_->getTileCount();
}

int NavMesh::GetPolyCount() const {
    if (!navMesh_) return 0;
    return navMesh_->getPolyCount();
}

void NavMesh::SetNavMesh(dtNavMesh* navMesh) {
    if (navMesh_ && navMesh_ != navMesh) {
        dtFreeNavMesh(navMesh_);
    }
    navMesh_ = navMesh;
}

NavMeshQuery::NavMeshQuery() {
    for (int i = 0; i < 32; ++i) {
        areaCosts_[i] = 1.0f;
    }
}

NavMeshQuery::~NavMeshQuery() {
    if (query_) {
        dtFreeNavMeshQuery(query_);
        query_ = nullptr;
    }
    if (filter_) {
        dtFreeQueryFilter(filter_);
        filter_ = nullptr;
    }
}

void NavMeshQuery::SetNavMesh(NavMesh* navMesh) {
    navMesh_ = navMesh;
    if (navMesh && navMesh->Get()) {
        if (!query_) {
            query_ = dtAllocNavMeshQuery();
        }
        if (!filter_) {
            filter_ = dtAllocQueryFilter();
        }
        if (query_) {
            query_->init(navMesh->Get(), 65536);
        }
    }
}

void NavMeshQuery::SetNavMesh(NavMesh& navMesh) {
    SetNavMesh(&navMesh);
}

NavMeshQueryResult NavMeshQuery::FindPath(const Math::Vec3f& start, const Math::Vec3f& end,
                                          PathData& path, PathFindingMode mode) {
    if (!query_ || !navMesh_ || !navMesh_->Get()) {
        return NavMeshQueryResult::Failure;
    }
    
    path.Clear();
    
    float startPos[3] = { start.x, start.y, start.z };
    float endPos[3] = { end.x, end.y, end.z };
    
    dtPolyRef startRef = 0;
    dtPolyRef endRef = 0;
    
    if (DT_FAILURE(query_->findNearestPoly(startPos, 2.0f, filter_, &startRef, nullptr))) {
        return NavMeshQueryResult::Failure;
    }
    if (DT_FAILURE(query_->findNearestPoly(endPos, 2.0f, filter_, &endRef, nullptr))) {
        return NavMeshQueryResult::Failure;
    }
    
    if (startRef == 0 || endRef == 0) {
        return NavMeshQueryResult::Failure;
    }
    
    const int maxPath = 256;
    dtPolyRef pathPolys[maxPath];
    float pathCorners[maxPath * 3];
    uint8_t pathFlags[maxPath];
    int numPolys = 0;
    int numCorners = 0;
    
    if (mode == PathFindingMode::Straight) {
        if (DT_SUCCESS(query_->findPath(startRef, endRef, startPos, endPos, pathPolys, &numPolys, maxPath))) {
            if (numPolys > 0) {
                if (DT_SUCCESS(query_->getPathCorner(pathPolys, numPolys, pathCorners, pathFlags, &numCorners, maxPath))) {
                    path.corners.resize(numCorners);
                    for (int i = 0; i < numCorners; ++i) {
                        path.corners[i] = { pathCorners[i * 3], pathCorners[i * 3 + 1], pathCorners[i * 3 + 2] };
                    }
                    
                    for (int i = 0; i < numCorners; ++i) {
                        PathPoint pt(path.corners[i]);
                        pt.flags = pathFlags[i];
                        path.waypoints.push_back(pt);
                    }
                }
            }
        }
    } else {
        if (DT_SUCCESS(query_->findPath(startRef, endRef, startPos, endPos, pathPolys, &numPolys, maxPath))) {
            if (numPolys > 0) {
                if (DT_SUCCESS(query_->getPathCorner(pathPolys, numPolys, pathCorners, pathFlags, &numCorners, maxPath))) {
                    path.corners.resize(numCorners);
                    for (int i = 0; i < numCorners; ++i) {
                        path.corners[i] = { pathCorners[i * 3], pathCorners[i * 3 + 1], pathCorners[i * 3 + 2] };
                    }
                    
                    for (int i = 0; i < numCorners; ++i) {
                        PathPoint pt(path.corners[i]);
                        pt.flags = pathFlags[i];
                        path.waypoints.push_back(pt);
                    }
                }
            }
        }
    }
    
    if (numPolys > 0 && numCorners > 0) {
        for (size_t i = 1; i < path.corners.size(); ++i) {
            path.pathLength += (path.corners[i] - path.corners[i - 1]).Length();
        }
        return NavMeshQueryResult::Success;
    }
    
    return NavMeshQueryResult::Failure;
}

NavMeshQueryResult NavMeshQuery::GetClosestPoint(const Math::Vec3f& position, Math::Vec3f& closest) {
    if (!query_ || !navMesh_ || !navMesh_->Get()) {
        return NavMeshQueryResult::Failure;
    }
    
    float pos[3] = { position.x, position.y, position.z };
    dtPolyRef ref = 0;
    float nearest[3];
    
    if (DT_SUCCESS(query_->findNearestPoly(pos, 2.0f, filter_, &ref, nearest))) {
        if (ref != 0) {
            closest = { nearest[0], nearest[1], nearest[2] };
            return NavMeshQueryResult::Success;
        }
    }
    
    return NavMeshQueryResult::Failure;
}

NavMeshQueryResult NavMeshQuery::GetRandomPoint(Math::Vec3f& randomPoint) {
    if (!query_ || !navMesh_ || !navMesh_->Get()) {
        return NavMeshQueryResult::Failure;
    }
    
    float randomPos[3];
    dtPolyRef ref = 0;
    
    if (DT_SUCCESS(query_->findRandomPoint(filter_, &ref, randomPos))) {
        randomPoint = { randomPos[0], randomPos[1], randomPos[2] };
        return NavMeshQueryResult::Success;
    }
    
    return NavMeshQueryResult::Failure;
}

NavMeshQueryResult NavMeshQuery::Raycast(const Math::Vec3f& start, const Math::Vec3f& end,
                                         float& hitT, Math::Vec3f& hitNormal) {
    if (!query_ || !navMesh_ || !navMesh_->Get()) {
        return NavMeshQueryResult::Failure;
    }
    
    float startPos[3] = { start.x, start.y, start.z };
    float endPos[3] = { end.x, end.y, end.z };
    
    dtPolyRef startRef = 0;
    if (DT_FAILURE(query_->findNearestPoly(startPos, 2.0f, filter_, &startRef, nullptr))) {
        return NavMeshQueryResult::Failure;
    }
    
    if (startRef == 0) {
        return NavMeshQueryResult::Failure;
    }
    
    const int maxPolys = 256;
    dtPolyRef polys[maxPolys];
    int numPolys = 0;
    
    float hitNormalOut[3] = { 0, 1, 0 };
    hitT = 0.0f;
    
    if (DT_SUCCESS(query_->raycast(startRef, startPos, endPos, filter_, &hitT, hitNormalOut, polys, &numPolys, maxPolys))) {
        hitNormal = { hitNormalOut[0], hitNormalOut[1], hitNormalOut[2] };
        return hitT <= 1.0f ? NavMeshQueryResult::Success : NavMeshQueryResult::Failure;
    }
    
    return NavMeshQueryResult::Failure;
}

NavMeshQueryResult NavMeshQuery::GetPathDistance(const Math::Vec3f& start, const Math::Vec3f& end, float& distance) {
    PathData path;
    NavMeshQueryResult result = FindPath(start, end, path);
    
    if (result == NavMeshQueryResult::Success) {
        distance = path.pathLength;
    } else {
        distance = (end - start).Length();
    }
    
    return result;
}

NavMeshQueryResult NavMeshQuery::GetPolysNearCircle(const Math::Vec3f& center, float radius,
                                                     std::vector<uint64_t>& polys) {
    if (!query_ || !navMesh_ || !navMesh_->Get()) {
        return NavMeshQueryResult::Failure;
    }
    
    float centerPos[3] = { center.x, center.y, center.z };
    const int maxPolys = 256;
    dtPolyRef polyRefs[maxPolys];
    int numPolys = 0;
    
    if (DT_SUCCESS(query_->findPolysAroundCircle(centerPos, radius, filter_, polyRefs, &numPolys, maxPolys))) {
        polys.resize(numPolys);
        for (int i = 0; i < numPolys; ++i) {
            polys[i] = polyRefs[i];
        }
        return NavMeshQueryResult::Success;
    }
    
    return NavMeshQueryResult::Failure;
}

NavMeshQueryResult NavMeshQuery::GetPolysNearBox(const Math::Vec3f& center, const Math::Vec3f& halfExtents,
                                                 const Math::Quatf& rotation, std::vector<uint64_t>& polys) {
    if (!query_ || !navMesh_ || !navMesh_->Get()) {
        return NavMeshQueryResult::Failure;
    }
    
    float centerPos[3] = { center.x, center.y, center.z };
    float extents[3] = { halfExtents.x, halfExtents.y, halfExtents.z };
    
    const int maxPolys = 256;
    dtPolyRef polyRefs[maxPolys];
    int numPolys = 0;
    
    if (DT_SUCCESS(query_->findPolysAroundBox(centerPos, extents, filter_, polyRefs, &numPolys, maxPolys))) {
        polys.resize(numPolys);
        for (int i = 0; i < numPolys; ++i) {
            polys[i] = polyRefs[i];
        }
        return NavMeshQueryResult::Success;
    }
    
    return NavMeshQueryResult::Failure;
}

void NavMeshQuery::SetAreaCost(int area, float cost) {
    if (area >= 0 && area < 32) {
        areaCosts_[area] = cost;
        if (filter_) {
            filter_->setAreaCost(area, cost);
        }
    }
}

}
}

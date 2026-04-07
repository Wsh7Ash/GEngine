#pragma once

// ================================================================
//  AOISpatialIndex.h
//  Spatial indexing for Area of Interest calculations.
// ================================================================

#include "../math/VecTypes.h"
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cmath>

namespace ge {
namespace net {

struct AOICell {
    std::unordered_set<uint32_t> entities;
};

struct AOISpatialIndex {
    float cellSize = 10.0f;
    std::unordered_map<int64_t, AOICell> cells;
    std::unordered_map<uint32_t, Math::Vec3f> entityPositions;
    std::unordered_map<uint32_t, float> entityRadii;

    int64_t GetCellKey(int32_t x, int32_t y, int32_t z) const {
        return (static_cast<int64_t>(x) << 40) |
               (static_cast<int64_t>(y) << 20) |
               static_cast<int64_t>(z);
    }

    Math::Vec3i WorldToCell(const Math::Vec3f& pos) const {
        return Math::Vec3i(
            static_cast<int32_t>(std::floor(pos.x / cellSize)),
            static_cast<int32_t>(std::floor(pos.y / cellSize)),
            static_cast<int32_t>(std::floor(pos.z / cellSize))
        );
    }

    void Insert(uint32_t entityId, const Math::Vec3f& position, float radius = 0.0f) {
        entityPositions[entityId] = position;
        entityRadii[entityId] = radius;

        Math::Vec3i cell = WorldToCell(position);
        int64_t key = GetCellKey(cell.x, cell.y, cell.z);
        cells[key].entities.insert(entityId);

        if (radius > 0.0f) {
            int32_t cellRadius = static_cast<int32_t>(std::ceil(radius / cellSize));
            for (int dx = -cellRadius; dx <= cellRadius; ++dx) {
                for (int dy = -cellRadius; dy <= cellRadius; ++dy) {
                    for (int dz = -cellRadius; dz <= cellRadius; ++dz) {
                        int64_t neighborKey = GetCellKey(cell.x + dx, cell.y + dy, cell.z + dz);
                        cells[neighborKey].entities.insert(entityId);
                    }
                }
            }
        }
    }

    void Remove(uint32_t entityId) {
        auto posIt = entityPositions.find(entityId);
        if (posIt != entityPositions.end()) {
            Math::Vec3i cell = WorldToCell(posIt->second);
            int64_t key = GetCellKey(cell.x, cell.y, cell.z);
            cells[key].entities.erase(entityId);
            entityPositions.erase(posIt);
        }
        entityRadii.erase(entityId);
    }

    void Update(uint32_t entityId, const Math::Vec3f& newPosition) {
        Remove(entityId);
        float radius = 0.0f;
        auto radIt = entityRadii.find(entityId);
        if (radIt != entityRadii.end()) {
            radius = radIt->second;
        }
        Insert(entityId, newPosition, radius);
    }

    std::vector<uint32_t> QueryRadius(const Math::Vec3f& center, float radius) const {
        std::vector<uint32_t> result;
        int32_t cellRadius = static_cast<int32_t>(std::ceil(radius / cellSize));
        Math::Vec3i centerCell = WorldToCell(center);

        for (int dx = -cellRadius; dx <= cellRadius; ++dx) {
            for (int dy = -cellRadius; dy <= cellRadius; ++dy) {
                for (int dz = -cellRadius; dz <= cellRadius; ++dz) {
                    int64_t key = GetCellKey(centerCell.x + dx, centerCell.y + dy, centerCell.z + dz);
                    auto it = cells.find(key);
                    if (it != cells.end()) {
                        for (uint32_t entityId : it->second.entities) {
                            auto posIt = entityPositions.find(entityId);
                            if (posIt != entityPositions.end()) {
                                Math::Vec3f diff = posIt->second - center;
                                float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                                float rad = 0.0f;
                                auto radIt = entityRadii.find(entityId);
                                if (radIt != entityRadii.end()) {
                                    rad = radIt->second;
                                }
                                if (distSq <= (radius + rad) * (radius + rad)) {
                                    result.push_back(entityId);
                                }
                            }
                        }
                    }
                }
            }
        }

        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
        return result;
    }

    std::vector<uint32_t> QueryBox(const Math::Vec3f& min, const Math::Vec3f& max) const {
        std::vector<uint32_t> result;
        Math::Vec3i minCell = WorldToCell(min);
        Math::Vec3i maxCell = WorldToCell(max);

        for (int x = minCell.x; x <= maxCell.x; ++x) {
            for (int y = minCell.y; y <= maxCell.y; ++y) {
                for (int z = minCell.z; z <= maxCell.z; ++z) {
                    int64_t key = GetCellKey(x, y, z);
                    auto it = cells.find(key);
                    if (it != cells.end()) {
                        result.insert(result.end(), it->second.entities.begin(), it->second.entities.end());
                    }
                }
            }
        }

        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
        return result;
    }

    void Clear() {
        cells.clear();
        entityPositions.clear();
        entityRadii.clear();
    }
};

} // namespace net
} // namespace ge
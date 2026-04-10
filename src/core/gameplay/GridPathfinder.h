#pragma once

#include "../math/VecTypes.h"
#include <algorithm>
#include <cstdint>
#include <queue>
#include <vector>

namespace ge {
namespace gameplay {

struct GridCoord {
    int x = 0;
    int y = 0;

    bool operator==(const GridCoord& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const GridCoord& other) const {
        return !(*this == other);
    }
};

struct GridPathData {
    int Width = 0;
    int Height = 0;
    float CellSize = 1.0f;
    Math::Vec2f Origin = {0.0f, 0.0f};
    std::vector<uint8_t> Blocked;

    void Resize(int width, int height, bool blocked = false) {
        Width = width;
        Height = height;
        Blocked.assign(static_cast<size_t>((std::max)(width, 0) * (std::max)(height, 0)), blocked ? 1u : 0u);
    }

    bool IsInBounds(const GridCoord& cell) const {
        return cell.x >= 0 && cell.x < Width && cell.y >= 0 && cell.y < Height;
    }

    int CellIndex(const GridCoord& cell) const {
        return cell.y * Width + cell.x;
    }

    bool IsBlocked(const GridCoord& cell) const {
        if (!IsInBounds(cell)) {
            return true;
        }
        const int index = CellIndex(cell);
        return index < 0 || index >= static_cast<int>(Blocked.size()) || Blocked[static_cast<size_t>(index)] != 0;
    }

    void SetBlocked(const GridCoord& cell, bool blocked) {
        if (!IsInBounds(cell)) {
            return;
        }
        Blocked[static_cast<size_t>(CellIndex(cell))] = blocked ? 1u : 0u;
    }

    Math::Vec2f CellToWorldCenter(const GridCoord& cell) const {
        return {
            Origin.x + (static_cast<float>(cell.x) + 0.5f) * CellSize,
            Origin.y + (static_cast<float>(cell.y) + 0.5f) * CellSize
        };
    }

    GridCoord WorldToCell(const Math::Vec2f& worldPosition) const {
        return {
            static_cast<int>((worldPosition.x - Origin.x) / CellSize),
            static_cast<int>((worldPosition.y - Origin.y) / CellSize)
        };
    }
};

class GridPathfinder {
public:
    static bool FindPath(const GridPathData& grid,
                         const GridCoord& start,
                         const GridCoord& goal,
                         std::vector<GridCoord>& outPath) {
        outPath.clear();

        if (!grid.IsInBounds(start) || !grid.IsInBounds(goal) || grid.IsBlocked(start) || grid.IsBlocked(goal)) {
            return false;
        }

        if (start == goal) {
            outPath.push_back(start);
            return true;
        }

        const int totalCells = grid.Width * grid.Height;
        std::vector<int> cameFrom(static_cast<size_t>(totalCells), -1);
        std::vector<uint8_t> visited(static_cast<size_t>(totalCells), 0);
        std::queue<GridCoord> frontier;

        frontier.push(start);
        visited[static_cast<size_t>(grid.CellIndex(start))] = 1;

        constexpr GridCoord directions[] = {
            {1, 0},
            {-1, 0},
            {0, 1},
            {0, -1}
        };

        while (!frontier.empty()) {
            const GridCoord current = frontier.front();
            frontier.pop();

            if (current == goal) {
                break;
            }

            for (const auto& direction : directions) {
                const GridCoord next = {current.x + direction.x, current.y + direction.y};
                if (!grid.IsInBounds(next) || grid.IsBlocked(next)) {
                    continue;
                }

                const int nextIndex = grid.CellIndex(next);
                if (visited[static_cast<size_t>(nextIndex)] != 0) {
                    continue;
                }

                visited[static_cast<size_t>(nextIndex)] = 1;
                cameFrom[static_cast<size_t>(nextIndex)] = grid.CellIndex(current);
                frontier.push(next);
            }
        }

        const int goalIndex = grid.CellIndex(goal);
        if (cameFrom[static_cast<size_t>(goalIndex)] == -1) {
            return false;
        }

        std::vector<GridCoord> reversedPath;
        GridCoord cursor = goal;
        reversedPath.push_back(cursor);

        while (cursor != start) {
            const int previousIndex = cameFrom[static_cast<size_t>(grid.CellIndex(cursor))];
            cursor = {
                previousIndex % grid.Width,
                previousIndex / grid.Width
            };
            reversedPath.push_back(cursor);
        }

        outPath.assign(reversedPath.rbegin(), reversedPath.rend());
        return true;
    }
};

} // namespace gameplay
} // namespace ge

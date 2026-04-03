#pragma once

// ================================================================
//  PredictionBuffer.h
//  Ring buffer for input history used in client-side prediction.
// ================================================================

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <array>
#include <cstring>
#include "../math/VecTypes.h"

namespace ge {
namespace net {

using EntityID = uint32_t;

static constexpr size_t DEFAULT_PREDICTION_BUFFER_SIZE = 256;

struct InputSnapshot {
    uint32_t tick = 0;
    Math::Vec3f moveAxis = Math::Vec3f::Zero();
    bool jump = false;
    bool crouch = false;
    bool sprint = false;
    bool attack = false;
    float timestamp = 0.0f;

    InputSnapshot() = default;

    InputSnapshot(uint32_t inTick, const Math::Vec3f& inMoveAxis, bool inJump, bool inCrouch, bool inSprint, bool inAttack = false, float inTimestamp = 0.0f)
        : tick(inTick), moveAxis(inMoveAxis), jump(inJump), crouch(inCrouch), sprint(inSprint), attack(inAttack), timestamp(inTimestamp) {}

    bool operator==(const InputSnapshot& other) const {
        return tick == other.tick &&
               moveAxis == other.moveAxis &&
               jump == other.jump &&
               crouch == other.crouch &&
               sprint == other.sprint &&
               attack == other.attack;
    }

    bool operator!=(const InputSnapshot& other) const {
        return !(*this == other);
    }
};

class PredictionBuffer {
public:
    static PredictionBuffer& Get();

    void Initialize(size_t maxEntities = 1000, size_t bufferSize = DEFAULT_PREDICTION_BUFFER_SIZE);
    void Shutdown();

    void ClearEntity(EntityID entityId);
    void ClearAll();

    void PushInput(EntityID entityId, const InputSnapshot& snapshot);
    const InputSnapshot* GetInput(EntityID entityId, uint32_t tick) const;
    const InputSnapshot* GetInputAtIndex(EntityID entityId, size_t index) const;

    bool HasInput(EntityID entityId, uint32_t tick) const;

    uint32_t GetOldestTick(EntityID entityId) const;
    uint32_t GetNewestTick(EntityID entityId) const;

    size_t GetBufferSize(EntityID entityId) const;
    size_t GetBufferCapacity() const { return bufferSize_; }

    void ClearBefore(EntityID entityId, uint32_t tick);
    void Trim(EntityID entityId, size_t maxHistory);

    std::vector<InputSnapshot> GetInputRange(EntityID entityId, uint32_t fromTick, uint32_t toTick) const;

    size_t GetTotalBufferedInputs() const;
    size_t GetEntityCount() const { return entityBuffers_.size(); }

private:
    PredictionBuffer() = default;
    ~PredictionBuffer() = default;

    struct InputRingBuffer {
        std::array<InputSnapshot, DEFAULT_PREDICTION_BUFFER_SIZE> buffer;
        size_t head = 0;
        size_t count = 0;
        uint32_t oldestTick = 0;
        uint32_t newestTick = 0;

        void Clear() {
            head = 0;
            count = 0;
            oldestTick = 0;
            newestTick = 0;
        }

        void Push(const InputSnapshot& snapshot) {
            buffer[head] = snapshot;
            head = (head + 1) % DEFAULT_PREDICTION_BUFFER_SIZE;
            if (count < DEFAULT_PREDICTION_BUFFER_SIZE) {
                count++;
                if (count == 1) {
                    oldestTick = snapshot.tick;
                }
            } else {
                oldestTick++;
            }
            newestTick = snapshot.tick;
        }

        const InputSnapshot* Get(uint32_t tick) const {
            if (count == 0) return nullptr;
            if (tick < oldestTick || tick > newestTick) return nullptr;

            size_t index = (head + count - 1) - (newestTick - tick);
            index %= DEFAULT_PREDICTION_BUFFER_SIZE;
            return &buffer[index];
        }

        std::vector<InputSnapshot> GetRange(uint32_t fromTick, uint32_t toTick) const {
            std::vector<InputSnapshot> result;
            if (count == 0) return result;
            if (fromTick < oldestTick) fromTick = oldestTick;
            if (toTick > newestTick) toTick = newestTick;
            if (fromTick > toTick) return result;

            result.reserve(toTick - fromTick + 1);
            for (uint32_t t = fromTick; t <= toTick; t++) {
                const InputSnapshot* snap = Get(t);
                if (snap) {
                    result.push_back(*snap);
                }
            }
            return result;
        }

        void ClearBefore(uint32_t tick) {
            if (count == 0 || tick <= oldestTick) return;
            if (tick > newestTick) {
                Clear();
                return;
            }
            size_t toRemove = tick - oldestTick;
            if (toRemove >= count) {
                Clear();
                return;
            }
            head = (head + toRemove) % DEFAULT_PREDICTION_BUFFER_SIZE;
            count -= toRemove;
            oldestTick = tick;
        }
    };

    size_t bufferSize_ = DEFAULT_PREDICTION_BUFFER_SIZE;
    std::unordered_map<EntityID, InputRingBuffer> entityBuffers_;
};

inline void PredictionBuffer::Initialize(size_t maxEntities, size_t bufferSize) {
    (void)maxEntities;
    bufferSize_ = bufferSize;
    entityBuffers_.reserve(maxEntities);
}

inline void PredictionBuffer::Shutdown() {
    ClearAll();
    entityBuffers_.clear();
}

inline void PredictionBuffer::ClearAll() {
    for (auto& pair : entityBuffers_) {
        pair.second.Clear();
    }
    entityBuffers_.clear();
}

inline void PredictionBuffer::ClearEntity(EntityID entityId) {
    auto it = entityBuffers_.find(entityId);
    if (it != entityBuffers_.end()) {
        it->second.Clear();
        entityBuffers_.erase(it);
    }
}

inline void PredictionBuffer::PushInput(EntityID entityId, const InputSnapshot& snapshot) {
    auto& buffer = entityBuffers_[entityId];
    buffer.Push(snapshot);
}

inline const InputSnapshot* PredictionBuffer::GetInput(EntityID entityId, uint32_t tick) const {
    auto it = entityBuffers_.find(entityId);
    if (it != entityBuffers_.end()) {
        return it->second.Get(tick);
    }
    return nullptr;
}

inline const InputSnapshot* PredictionBuffer::GetInputAtIndex(EntityID entityId, size_t index) const {
    auto it = entityBuffers_.find(entityId);
    if (it != entityBuffers_.end() && index < it->second.count) {
        size_t actualIndex = (it->second.head + index) % DEFAULT_PREDICTION_BUFFER_SIZE;
        return &it->second.buffer[actualIndex];
    }
    return nullptr;
}

inline bool PredictionBuffer::HasInput(EntityID entityId, uint32_t tick) const {
    return GetInput(entityId, tick) != nullptr;
}

inline uint32_t PredictionBuffer::GetOldestTick(EntityID entityId) const {
    auto it = entityBuffers_.find(entityId);
    if (it != entityBuffers_.end()) {
        return it->second.oldestTick;
    }
    return 0;
}

inline uint32_t PredictionBuffer::GetNewestTick(EntityID entityId) const {
    auto it = entityBuffers_.find(entityId);
    if (it != entityBuffers_.end()) {
        return it->second.newestTick;
    }
    return 0;
}

inline size_t PredictionBuffer::GetBufferSize(EntityID entityId) const {
    auto it = entityBuffers_.find(entityId);
    if (it != entityBuffers_.end()) {
        return it->second.count;
    }
    return 0;
}

inline void PredictionBuffer::ClearBefore(EntityID entityId, uint32_t tick) {
    auto it = entityBuffers_.find(entityId);
    if (it != entityBuffers_.end()) {
        it->second.ClearBefore(tick);
        if (it->second.count == 0) {
            entityBuffers_.erase(it);
        }
    }
}

inline void PredictionBuffer::Trim(EntityID entityId, size_t maxHistory) {
    auto it = entityBuffers_.find(entityId);
    if (it != entityBuffers_.end() && it->second.count > maxHistory) {
        uint32_t targetTick = it->second.newestTick - static_cast<uint32_t>(maxHistory) + 1;
        ClearBefore(entityId, targetTick);
    }
}

inline std::vector<InputSnapshot> PredictionBuffer::GetInputRange(EntityID entityId, uint32_t fromTick, uint32_t toTick) const {
    auto it = entityBuffers_.find(entityId);
    if (it != entityBuffers_.end()) {
        return it->second.GetRange(fromTick, toTick);
    }
    return {};
}

inline size_t PredictionBuffer::GetTotalBufferedInputs() const {
    size_t total = 0;
    for (const auto& pair : entityBuffers_) {
        total += pair.second.count;
    }
    return total;
}

} // namespace net
} // namespace ge

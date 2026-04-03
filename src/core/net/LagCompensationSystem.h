#pragma once

// ================================================================
//  LagCompensationSystem.h
//  Core lag compensation system for accurate hit detection.
// ================================================================

#include "StateRecorder.h"
#include "NetworkManager.h"
#include "GameState.h"
#include "../ecs/World.h"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/Physics3D_API.h"
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>
#include <chrono>

namespace ge {
namespace ecs {
class World;
} // namespace ecs

namespace net {

using EntityID = uint32_t;

enum class HitShape : uint8_t {
    Ray = 0,
    Sphere = 1,
    Box = 2,
    Capsule = 3
};

struct HitQueryParams {
    EntityID shooterEntityId = 0;
    uint32_t clientTick = 0;
    Math::Vec3f origin = Math::Vec3f::Zero();
    Math::Vec3f direction = Math::Vec3f::Zero();
    float maxDistance = 100.0f;
    float damage = 10.0f;
    HitShape hitShape = HitShape::Ray;
    float sphereRadius = 0.1f;
    Math::Vec3f boxHalfExtents = Math::Vec3f::Zero();
    float capsuleRadius = 0.1f;
    float capsuleHalfHeight = 0.5f;
    int collisionLayer = 1;
    int collisionMask = -1;
    uint32_t excludeEntityId = 0;

    void Serialize(NetworkSerializer& serializer) const {
        serializer.WriteUInt32(shooterEntityId);
        serializer.WriteUInt32(clientTick);
        serializer.WriteBytes(&origin, sizeof(Math::Vec3f));
        serializer.WriteBytes(&direction, sizeof(Math::Vec3f));
        serializer.WriteFloat(maxDistance);
        serializer.WriteFloat(damage);
        serializer.WriteUInt8(static_cast<uint8_t>(hitShape));
        serializer.WriteFloat(sphereRadius);
        serializer.WriteBytes(&boxHalfExtents, sizeof(Math::Vec3f));
        serializer.WriteFloat(capsuleRadius);
        serializer.WriteFloat(capsuleHalfHeight);
        serializer.WriteInt32(collisionLayer);
        serializer.WriteInt32(collisionMask);
        serializer.WriteUInt32(excludeEntityId);
    }

    void Deserialize(NetworkDeserializer& deserializer) {
        shooterEntityId = deserializer.ReadUInt32();
        clientTick = deserializer.ReadUInt32();
        deserializer.ReadBytes(&origin, sizeof(Math::Vec3f));
        deserializer.ReadBytes(&direction, sizeof(Math::Vec3f));
        maxDistance = deserializer.ReadFloat();
        damage = deserializer.ReadFloat();
        hitShape = static_cast<HitShape>(deserializer.ReadUInt8());
        sphereRadius = deserializer.ReadFloat();
        deserializer.ReadBytes(&boxHalfExtents, sizeof(Math::Vec3f));
        capsuleRadius = deserializer.ReadFloat();
        capsuleHalfHeight = deserializer.ReadFloat();
        collisionLayer = deserializer.ReadInt32();
        collisionMask = deserializer.ReadInt32();
        excludeEntityId = deserializer.ReadUInt32();
    }
};

struct HitResult {
    EntityID hitEntity = 0;
    Math::Vec3f hitPosition = Math::Vec3f::Zero();
    Math::Vec3f hitNormal = Math::Vec3f::Zero();
    float distance = 0.0f;
    float damage = 0.0f;
    bool didHit = false;
    uint32_t serverTick = 0;
    bool wasLagCompensated = false;
    bool wasInterpolated = false;

    void Serialize(NetworkSerializer& serializer) const {
        serializer.WriteUInt32(hitEntity);
        serializer.WriteBytes(&hitPosition, sizeof(Math::Vec3f));
        serializer.WriteBytes(&hitNormal, sizeof(Math::Vec3f));
        serializer.WriteFloat(distance);
        serializer.WriteFloat(damage);
        serializer.WriteBool(didHit);
        serializer.WriteUInt32(serverTick);
        serializer.WriteBool(wasLagCompensated);
        serializer.WriteBool(wasInterpolated);
    }

    void Deserialize(NetworkDeserializer& deserializer) {
        hitEntity = deserializer.ReadUInt32();
        deserializer.ReadBytes(&hitPosition, sizeof(Math::Vec3f));
        deserializer.ReadBytes(&hitNormal, sizeof(Math::Vec3f));
        distance = deserializer.ReadFloat();
        damage = deserializer.ReadFloat();
        didHit = deserializer.ReadBool();
        serverTick = deserializer.ReadUInt32();
        wasLagCompensated = deserializer.ReadBool();
        wasInterpolated = deserializer.ReadBool();
    }
};

struct LagCompensationStats {
    uint32_t rewindTick = 0;
    uint32_t currentTick = 0;
    size_t entitiesRewound = 0;
    float queryTimeMs = 0.0f;
    bool wasSuccessful = false;
    float positionError = 0.0f;
};

struct LagCompensationConfig {
    uint32_t maxHistorySize = 512;
    uint32_t minHistoryRequired = 60;
    float positionTolerance = 0.1f;
    float extrapolationTime = 0.1f;
    bool enableInterpolation = true;
    bool enableExtrapolation = true;
    bool strictRewind = false;
};

class LagCompensationSystem {
public:
    static LagCompensationSystem& Get();

    void Initialize(ecs::World* world, NetworkManager* netMgr, StateRecorder* recorder);
    void Shutdown();

    void Update(float dt);

    void SetConfig(const LagCompensationConfig& config);
    const LagCompensationConfig& GetConfig() const { return config_; }

    HitResult ProcessHitQuery(uint32_t clientId, const HitQueryParams& params);
    HitResult ProcessHitQueryAtTick(const HitQueryParams& params, uint32_t targetTick);

    uint32_t CalculateRewindTick(uint32_t clientTick, float clientLatency) const;
    uint32_t CalculateRewindTickFromRTT(float rtt) const;

    bool RewindToTick(uint32_t tick);
    void RestoreFromRewind();

    bool IsRewinding() const { return isRewinding_; }
    uint32_t GetCurrentRewindTick() const { return currentRewindTick_; }

    std::vector<HitResult> ExecuteHistoricalRaycast(const Math::Vec3f& origin, const Math::Vec3f& direction, float maxDistance);
    std::vector<HitResult> ExecuteHistoricalSweepSphere(const Math::Vec3f& start, const Math::Vec3f& end, float radius);
    std::vector<HitResult> ExecuteHistoricalSweepBox(const Math::Vec3f& start, const Math::Vec3f& end, const Math::Vec3f& halfExtents);
    std::vector<HitResult> ExecuteHistoricalOverlapSphere(const Math::Vec3f& position, float radius);
    std::vector<HitResult> ExecuteHistoricalOverlapBox(const Math::Vec3f& position, const Math::Vec3f& halfExtents);

    std::function<void(uint32_t, const HitResult&)> onHitDetected;
    std::function<void(uint32_t, const LagCompensationStats&)> onStatsUpdated;

    const StateRecorder& GetStateRecorder() const { return *stateRecorder_; }
    StateRecorder& GetStateRecorder() { return *stateRecorder_; }

    size_t GetTotalQueries() const { return totalQueries_; }
    size_t GetSuccessfulQueries() const { return successfulQueries_; }
    float GetAverageQueryTime() const { return averageQueryTime_; }

private:
    LagCompensationSystem();
    ~LagCompensationSystem() = default;

    bool ValidateRewindTick(uint32_t tick) const;
    void SaveCurrentEntityStates();
    void ApplyHistoricalEntityStates(uint32_t tick, float interpolationFraction);
    void RestoreEntityStates();

    struct SavedEntityState {
        EntityID entityId;
        Math::Vec3f position;
        Math::Quatf rotation;
    };

    ecs::World* world_ = nullptr;
    NetworkManager* networkManager_ = nullptr;
    StateRecorder* stateRecorder_ = nullptr;

    LagCompensationConfig config_;
    bool isRewinding_ = false;
    uint32_t currentRewindTick_ = 0;
    float currentInterpolationFraction_ = 0.0f;

    std::vector<SavedEntityState> savedEntityStates_;
    std::vector<EntityID> modifiedEntities_;

    std::chrono::high_resolution_clock::time_point queryStartTime_;

    size_t totalQueries_ = 0;
    size_t successfulQueries_ = 0;
    float averageQueryTime_ = 0.0f;
    float totalQueryTime_ = 0.0f;
};

} // namespace net
} // namespace ge

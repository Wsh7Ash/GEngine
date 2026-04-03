#pragma once

// ================================================================
//  CharacterState.h
//  Serializable snapshot of CharacterVirtual state for prediction.
// ================================================================

#include "NetworkSerializer.h"
#include "../math/VecTypes.h"
#include "../math/quaternion.h"
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace ge {
namespace net {

struct CharacterStateSnapshot {
    uint32_t tick = 0;
    Math::Vec3f position = Math::Vec3f::Zero();
    Math::Vec3f velocity = Math::Vec3f::Zero();
    Math::Quatf rotation = Math::Quatf::Identity();
    bool isGrounded = false;
    float maxSlopeAngle = 45.0f;
    uint32_t checksum = 0;

    CharacterStateSnapshot() = default;

    CharacterStateSnapshot(uint32_t inTick, const Math::Vec3f& inPosition, 
                           const Math::Vec3f& inVelocity, const Math::Quatf& inRotation,
                           bool inIsGrounded, float inMaxSlopeAngle = 45.0f)
        : tick(inTick), position(inPosition), velocity(inVelocity), 
          rotation(inRotation), isGrounded(inIsGrounded), maxSlopeAngle(inMaxSlopeAngle) {
        ComputeChecksum();
    }

    void Serialize(NetworkSerializer& serializer) const {
        serializer.WriteUInt32(tick);
        serializer.WriteBytes(&position, sizeof(Math::Vec3f));
        serializer.WriteBytes(&velocity, sizeof(Math::Vec3f));
        serializer.WriteBytes(&rotation, sizeof(Math::Quatf));
        serializer.WriteBool(isGrounded);
        serializer.WriteFloat(maxSlopeAngle);
        serializer.WriteUInt32(checksum);
    }

    void Deserialize(NetworkDeserializer& deserializer) {
        tick = deserializer.ReadUInt32();
        deserializer.ReadBytes(&position, sizeof(Math::Vec3f));
        deserializer.ReadBytes(&velocity, sizeof(Math::Vec3f));
        deserializer.ReadBytes(&rotation, sizeof(Math::Quatf));
        isGrounded = deserializer.ReadBool();
        maxSlopeAngle = deserializer.ReadFloat();
        checksum = deserializer.ReadUInt32();
    }

    void ComputeChecksum() {
        uint32_t hash = 2166136261u;
        
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&tick);
        for (size_t i = 0; i < sizeof(tick); ++i) {
            hash ^= bytes[i];
            hash *= 16777619u;
        }
        
        bytes = reinterpret_cast<const uint8_t*>(&position);
        for (size_t i = 0; i < sizeof(position); ++i) {
            hash ^= bytes[i];
            hash *= 16777619u;
        }
        
        bytes = reinterpret_cast<const uint8_t*>(&velocity);
        for (size_t i = 0; i < sizeof(velocity); ++i) {
            hash ^= bytes[i];
            hash *= 16777619u;
        }
        
        bytes = reinterpret_cast<const uint8_t*>(&rotation);
        for (size_t i = 0; i < sizeof(rotation); ++i) {
            hash ^= bytes[i];
            hash *= 16777619u;
        }
        
        bytes = reinterpret_cast<const uint8_t*>(&isGrounded);
        for (size_t i = 0; i < sizeof(isGrounded); ++i) {
            hash ^= bytes[i];
            hash *= 16777619u;
        }
        
        bytes = reinterpret_cast<const uint8_t*>(&maxSlopeAngle);
        for (size_t i = 0; i < sizeof(maxSlopeAngle); ++i) {
            hash ^= bytes[i];
            hash *= 16777619u;
        }
        
        checksum = hash;
    }

    bool ValidateChecksum() const {
        CharacterStateSnapshot temp = *this;
        temp.checksum = 0;
        temp.ComputeChecksum();
        return temp.checksum == checksum;
    }

    bool Matches(const CharacterStateSnapshot& other, 
                 float positionThreshold = 0.01f, 
                 float velocityThreshold = 0.01f) const {
        if (tick != other.tick) return false;
        
        float posDist = (position - other.position).Length();
        if (posDist > positionThreshold) return false;
        
        float velDist = (velocity - other.velocity).Length();
        if (velDist > velocityThreshold) return false;
        
        float dot = rotation.Dot(other.rotation);
        float clampedDot = dot < -1.0f ? -1.0f : (dot > 1.0f ? 1.0f : dot);
        float rotDiff = std::acos(clampedDot) * 114.592f;
        if (rotDiff > 1.0f) return false;
        
        return true;
    }

    bool operator==(const CharacterStateSnapshot& other) const {
        return tick == other.tick && 
               position == other.position && 
               velocity == other.velocity && 
               rotation == other.rotation && 
               isGrounded == other.isGrounded;
    }

    bool operator!=(const CharacterStateSnapshot& other) const {
        return !(*this == other);
    }

    static size_t SerializedSize() {
        return sizeof(tick) + sizeof(Math::Vec3f) + sizeof(Math::Vec3f) + 
               sizeof(Math::Quatf) + sizeof(isGrounded) + sizeof(maxSlopeAngle) + sizeof(checksum);
    }
};

struct PredictionConfig {
    uint32_t tickRate = 60;
    uint32_t maxInputHistory = 256;
    float positionThreshold = 0.1f;
    float velocityThreshold = 0.1f;
    float rotationThreshold = 2.0f;
    float reconciliationBlendTime = 0.1f;
    uint32_t maxReconciliationTicks = 20;
    bool enableRollback = true;
    bool enableSmoothing = true;
    bool enableExtrapolation = true;
    float extrapolationTime = 0.1f;
};

} // namespace net
} // namespace ge

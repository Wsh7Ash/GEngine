#pragma once

// ================================================================
//  System.h
//  Base class for ECS systems with dependency tracking.
// ================================================================

#include "Entity.h"
#include "ComponentRegistry.h"
#include <bitset>
#include <set>
#include <vector>
#include <functional>
#include <thread>

namespace ge {
namespace ecs
{

class World;

using Signature = std::bitset<MAX_COMPONENTS>;

// Forward declarations
class SystemExecutor;
class SystemGraph;

enum class SystemPriority {
    Critical = 0,   // Physics, input - must run first
    High = 1,        // Animation, AI
    Normal = 2,      // Most systems
    Low = 3,        // Particles
    Background = 4   // Async loading
};

/**
 * @brief Base class for all Systems.
 * 
 * Systems declare their component dependencies using:
 * - ReadSignature(): Components read but not written
 * - WriteSignature(): Components written
 * 
 * The SystemExecutor analyzes these to parallelize execution.
 * 
 * @note Thread Safety: Systems execute in parallel via SystemExecutor.
 *       Do not access World outside of Update(). Do not maintain
 *       mutable shared state. See docs/THREADING_CONTRACT.md.
 * 
 * @par Example:
 * @code
 * class VelocitySystem : public System {
 *     Signature GetReadSignature() const override {
 *         Signature sig;
 *         sig.set(GetComponentTypeID<TransformComponent>(), true);
 *         return sig;
 *     }
 *     
 *     Signature GetWriteSignature() const override {
 *         Signature sig;
 *         sig.set(GetComponentTypeID<VelocityComponent>(), true);
 *         return sig;
 *     }
 *     
 *     void Update(World& world, float dt) override {
 *         for (Entity e : entities) {
 *             auto& t = world.GetComponent<TransformComponent>(e);
 *             auto& v = world.GetComponent<VelocityComponent>(e);
 *             t.position += v.value * dt;
 *         }
 *     }
 * };
 * @endcode
 * 
 * @see docs/THREADING_CONTRACT.md
 */
class System
{
public:
    virtual ~System() = default;

    /// Entities matching the system's signature.
    std::set<Entity> entities;

    /// Get the read signature (components this system reads).
    virtual Signature GetReadSignature() const { return readSignature_; }
    
    /// Get the write signature (components this system writes).
    virtual Signature GetWriteSignature() const { return writeSignature_; }
    
    /// Set read signature (usually called during registration).
    void SetReadSignature(Signature sig) { readSignature_ = sig; }
    
    /// Set write signature (usually called during registration).
    void SetWriteSignature(Signature sig) { writeSignature_ = sig; }
    
    /// Get system priority.
    virtual SystemPriority GetPriority() const { return SystemPriority::Normal; }
    
    /// Whether this system supports parallel execution.
    virtual bool IsParallelizable() const { return true; }
    
    /// Get estimated execution time (for load balancing).
    virtual float GetEstimatedTime() const { return 1.0f; }

protected:
    Signature readSignature_;
    Signature writeSignature_;
    
    friend class SystemExecutor;
    friend class SystemGraph;
};

} // namespace ecs
} // namespace ge

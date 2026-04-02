#include "ManagedScriptComponent.h"
#include "../../debug/log.h"

namespace ge {
namespace scripting {

void ManagedScriptInstanceWrapper::OnCreate() {
    GE_LOG_INFO("ManagedScriptInstanceWrapper::OnCreate - Entity: %llu", entity_.value);
    if (createCallback_) {
        createCallback_();
    }
}

void ManagedScriptInstanceWrapper::OnUpdate(float dt) {
    if (updateCallback_) {
        updateCallback_(dt);
    }
}

void ManagedScriptInstanceWrapper::OnDestroy() {
    GE_LOG_INFO("ManagedScriptInstanceWrapper::OnDestroy - Entity: %llu", entity_.value);
    if (destroyCallback_) {
        destroyCallback_();
    }
}

void ManagedScriptInstanceWrapper::OnCollisionEnter(ecs::Entity other) {
    GE_LOG_INFO("ManagedScriptInstanceWrapper::OnCollisionEnter - Entity: %llu, Other: %llu", 
                entity_.value, other.value);
    if (collisionEnterCallback_) {
        collisionEnterCallback_(other.value);
    }
}

void ManagedScriptInstanceWrapper::OnCollisionExit(ecs::Entity other) {
    GE_LOG_INFO("ManagedScriptInstanceWrapper::OnCollisionExit - Entity: %llu, Other: %llu", 
                entity_.value, other.value);
    if (collisionExitCallback_) {
        collisionExitCallback_(other.value);
    }
}

void ManagedScriptInstanceWrapper::OnTriggerEnter(ecs::Entity other) {
    GE_LOG_INFO("ManagedScriptInstanceWrapper::OnTriggerEnter - Entity: %llu, Other: %llu", 
                entity_.value, other.value);
    if (triggerEnterCallback_) {
        triggerEnterCallback_(other.value);
    }
}

void ManagedScriptInstanceWrapper::OnTriggerExit(ecs::Entity other) {
    GE_LOG_INFO("ManagedScriptInstanceWrapper::OnTriggerExit - Entity: %llu, Other: %llu", 
                entity_.value, other.value);
    if (triggerExitCallback_) {
        triggerExitCallback_(other.value);
    }
}

void ManagedScriptInstanceWrapper::OnTransformInterpolate(const Math::Vec3f& interpolatedPos, float alpha) {
    (void)interpolatedPos;
    (void)alpha;
}

} // namespace scripting
} // namespace ge
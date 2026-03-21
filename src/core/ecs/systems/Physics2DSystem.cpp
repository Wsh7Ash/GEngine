#include "Physics2DSystem.h"
#include "../../ecs/World.h"
#include "../../ecs/components/TransformComponent.h"
#include "../../ecs/components/Rigidbody2DComponent.h"
#include "../../ecs/components/BoxCollider2DComponent.h"
#include "../../ecs/components/NativeScriptComponent.h"
#include "../../ecs/ScriptableEntity.h"
#include "../../editor/EditorToolbar.h"
#include <box2d/box2d.h>

namespace ge {
namespace ecs {

Physics2DSystem::Physics2DSystem() {
    physics_world_ = std::make_unique<b2World>(b2Vec2(0.0f, gravity_));
    physics_world_->SetContactListener(this);
}

Physics2DSystem::~Physics2DSystem() = default;

void Physics2DSystem::Update(World& world, float ts) {
    bool isPlaying = (editor::EditorToolbar::GetState() == editor::SceneState::Play);

    // 1. Synchronize/Create Bodies
    for (auto entity : entities) {
        auto& rb = world.GetComponent<Rigidbody2DComponent>(entity);
        auto& tc = world.GetComponent<TransformComponent>(entity);

        b2Body* body = (b2Body*)rb.RuntimeBody;

        if (!body) {
            // Create Body
            b2BodyDef bodyDef;
            bodyDef.type = (b2BodyType)rb.Type;
            bodyDef.position.Set(tc.position.x, tc.position.y);
            bodyDef.angle = tc.rotation.ToEuler().z; 
            bodyDef.fixedRotation = rb.FixedRotation;
            bodyDef.userData.pointer = (uintptr_t)entity.GetIndex();

            body = physics_world_->CreateBody(&bodyDef);
            rb.RuntimeBody = body;

            // Add Fixtures (Colliders)
            if (world.HasComponent<BoxCollider2DComponent>(entity)) {
                auto& bc = world.GetComponent<BoxCollider2DComponent>(entity);
                
                b2PolygonShape boxShape;
                boxShape.SetAsBox(bc.Size.x * tc.scale.x, bc.Size.y * tc.scale.y, 
                                  b2Vec2(bc.Offset.x, bc.Offset.y), 0.0f);

                b2FixtureDef fixtureDef;
                fixtureDef.shape = &boxShape;
                fixtureDef.density = bc.Density;
                fixtureDef.friction = bc.Friction;
                fixtureDef.restitution = bc.Restitution;
                fixtureDef.restitutionThreshold = bc.RestitutionThreshold;
                
                bc.RuntimeFixture = body->CreateFixture(&fixtureDef);
            }
        }

        // 2. Sync during Editor Mode (or if Transform was changed via gizmo)
        if (!isPlaying) {
            body->SetTransform(b2Vec2(tc.position.x, tc.position.y), tc.rotation.ToEuler().z);
        }
    }

    // 3. Step Simulation
    if (isPlaying) {
        current_world_ = &world;
        physics_world_->Step(ts, velocity_iterations_, position_iterations_);
        current_world_ = nullptr;

        // 4. Sync physics results back to ECS
        for (auto entity : entities) {
            auto& rb = world.GetComponent<Rigidbody2DComponent>(entity);
            auto& tc = world.GetComponent<TransformComponent>(entity);

            b2Body* body = (b2Body*)rb.RuntimeBody;
            if (body && rb.Type != RigidBody2DType::Static) {
                const auto& position = body->GetPosition();
                tc.position.x = position.x;
                tc.position.y = position.y;
                
                float angle = body->GetAngle();
                tc.rotation = Math::Quatf::FromEuler({0.0f, 0.0f, angle});
            }
        }
    }
}

void Physics2DSystem::BeginContact(b2Contact* contact) {
    if (!current_world_) return;

    Entity entityA((uint32_t)contact->GetFixtureA()->GetBody()->GetUserData().pointer);
    Entity entityB((uint32_t)contact->GetFixtureB()->GetBody()->GetUserData().pointer);

    bool isSensorA = contact->GetFixtureA()->IsSensor();
    bool isSensorB = contact->GetFixtureB()->IsSensor();

    if (current_world_->HasComponent<NativeScriptComponent>(entityA)) {
        auto& ns = current_world_->GetComponent<NativeScriptComponent>(entityA);
        if (ns.instance) {
            if (isSensorA || isSensorB)
                ns.instance->OnTriggerEnter(entityB);
            else
                ns.instance->OnCollisionEnter(entityB);
        }
    }

    if (current_world_->HasComponent<NativeScriptComponent>(entityB)) {
        auto& ns = current_world_->GetComponent<NativeScriptComponent>(entityB);
        if (ns.instance) {
            if (isSensorA || isSensorB)
                ns.instance->OnTriggerEnter(entityA);
            else
                ns.instance->OnCollisionEnter(entityA);
        }
    }
}

void Physics2DSystem::EndContact(b2Contact* contact) {
    if (!current_world_) return;

    Entity entityA((uint32_t)contact->GetFixtureA()->GetBody()->GetUserData().pointer);
    Entity entityB((uint32_t)contact->GetFixtureB()->GetBody()->GetUserData().pointer);

    bool isSensorA = contact->GetFixtureA()->IsSensor();
    bool isSensorB = contact->GetFixtureB()->IsSensor();

    if (current_world_->HasComponent<NativeScriptComponent>(entityA)) {
        auto& ns = current_world_->GetComponent<NativeScriptComponent>(entityA);
        if (ns.instance) {
            if (isSensorA || isSensorB)
                ns.instance->OnTriggerExit(entityB);
            else
                ns.instance->OnCollisionExit(entityB);
        }
    }

    if (current_world_->HasComponent<NativeScriptComponent>(entityB)) {
        auto& ns = current_world_->GetComponent<NativeScriptComponent>(entityB);
        if (ns.instance) {
            if (isSensorA || isSensorB)
                ns.instance->OnTriggerExit(entityA);
            else
                ns.instance->OnCollisionExit(entityA);
        }
    }
}

} // namespace ecs
} // namespace ge

#include <Jolt/Jolt.h>

// Jolt includes must be after Jolt.h
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/IssueReporting.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ContactListener.h>

#include "Physics3DSystem.h"
#include "../World.h"
#include "../components/TransformComponent.h"
#include "../components/Rigidbody3DComponent.h"
#include "../components/Collider3DComponent.h"
#include "../components/MeshComponent.h"
#include "../../renderer/Mesh.h"
#include "../components/NativeScriptComponent.h"
#include "../../debug/log.h"
#include "../../debug/assert.h"

#include <iostream>
#include <stdarg.h>
#include <thread>

#include "../../editor/EditorToolbar.h"

namespace ge {
namespace ecs {

namespace {

    namespace Layers {
        static constexpr JPH::ObjectLayer NON_MOVING = 0;
        static constexpr JPH::ObjectLayer MOVING = 1;
        static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
    };

    namespace BroadPhaseLayers {
        static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
        static constexpr JPH::BroadPhaseLayer MOVING(1);
        static constexpr JPH::uint NUM_LAYERS(2);
    };

    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
    public:
        BPLayerInterfaceImpl() {
            mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
            mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
        }
        virtual JPH::uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }
        virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
            return mObjectToBroadPhase[inLayer];
        }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
            switch ((JPH::BroadPhaseLayer::Type)inLayer) {
                case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
                case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING: return "MOVING";
                default: return "INVALID";
            }
        }
#endif
    private:
        JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
    };

    class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
            switch (inLayer1) {
                case Layers::NON_MOVING: return inLayer2 == BroadPhaseLayers::MOVING;
                case Layers::MOVING: return true;
                default: return false;
            }
        }
    };

    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
            switch (inObject1) {
                case Layers::NON_MOVING: return inObject2 == Layers::MOVING;
                case Layers::MOVING: return true;
                default: return false;
            }
        }
    };

    BPLayerInterfaceImpl broad_phase_layer_interface;
    ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
    ObjectLayerPairFilterImpl object_vs_object_layer_filter;

    class Physics3DContactListener : public JPH::ContactListener {
    public:
        Physics3DContactListener(World& world) : m_World(world) {}

        virtual JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override {
            return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
        }

        virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override {
            Entity e1 = (Entity)inBody1.GetUserData();
            Entity e2 = (Entity)inBody2.GetUserData();

            if (m_World.IsAlive(e1) && m_World.HasComponent<NativeScriptComponent>(e1)) {
                auto& nsc = m_World.GetComponent<NativeScriptComponent>(e1);
                if (nsc.instance) nsc.instance->OnCollisionEnter(e2);
            }
            if (m_World.IsAlive(e2) && m_World.HasComponent<NativeScriptComponent>(e2)) {
                auto& nsc = m_World.GetComponent<NativeScriptComponent>(e2);
                if (nsc.instance) nsc.instance->OnCollisionEnter(e1);
            }
        }

        virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override {}

        virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override {
            // Note: Jolt OnContactRemoved only provides BodyIDs, not full Body objects.
            // We'd need to fetch them if we want to trigger Exit callbacks.
            // For now, we'll keep it simple.
        }

    private:
        World& m_World;
    };


} // anonymous namespace

    // Callback for Jolt logging
    static void TraceImpl(const char *inFMT, ...) {
        va_list list;
        va_start(list, inFMT);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), inFMT, list);
        va_end(list);
        GE_LOG_INFO("[Jolt] %s", buffer);
    }

#ifdef JPH_ENABLE_ASSERTS
    static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, uint32_t inLine) {
        GE_LOG_CRITICAL("[Jolt] Assertion failed: %s (%s) at %s:%d", inExpression, (inMessage ? inMessage : ""), inFile, (int)inLine);
        return true; // Abort
    }
#endif

    Physics3DSystem::Physics3DSystem() {
        InitializeJolt();
    }

    Physics3DSystem::~Physics3DSystem() {
        ShutdownJolt();
    }

    void Physics3DSystem::InitializeJolt() {
        // Register allocation hook
        JPH::RegisterDefaultAllocator();

        // Install trace and assert callbacks
        JPH::Trace = TraceImpl;
        JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)

        // Create factory
        JPH::Factory::sInstance = new JPH::Factory();

        // Register all Jolt physics types
        JPH::RegisterTypes();

        // Job System
        m_JobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, (int)std::thread::hardware_concurrency() - 1);

        // Temp Allocator
        m_TempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024); // 10 MB

        // Create the physics system
        m_PhysicsSystem = new JPH::PhysicsSystem();
        m_PhysicsSystem->Init(1024, 0, 1024, 1024, broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);
        
        // Contact Listener
        // Note: We'll initialize this with a world in Update or during first creation if possible.
        // Actually, we need the World reference. For now, we'll defer registration to Update or pass it in.
        
        GE_LOG_INFO("Physics3DSystem initialized (Jolt v5.0.0)");
    }

    void Physics3DSystem::ShutdownJolt() {
        delete m_PhysicsSystem;
        delete m_TempAllocator;
        delete m_JobSystem;
        if (m_ContactListener) delete m_ContactListener;
        
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    void Physics3DSystem::Update(World& world, float dt) {
        if (!m_PhysicsSystem) return;

        bool isPlaying = (editor::EditorToolbar::GetState() == editor::SceneState::Play);
        JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

        // 0. Initialize Contact Listener if in Play mode and not yet created
        if (isPlaying && !m_ContactListener) {
            m_ContactListener = new Physics3DContactListener(world);
            m_PhysicsSystem->SetContactListener(m_ContactListener);
        } else if (!isPlaying && m_ContactListener) {
            m_PhysicsSystem->SetContactListener(nullptr);
            delete m_ContactListener;
            m_ContactListener = nullptr;
        }

        // 1. Synchronize/Create Bodies
        for (auto entity : entities) {
            if (!world.HasComponent<Rigidbody3DComponent>(entity) || !world.HasComponent<TransformComponent>(entity)) {
                continue;
            }

            auto& rb = world.GetComponent<Rigidbody3DComponent>(entity);
            auto& tc = world.GetComponent<TransformComponent>(entity);
            
            JPH::BodyID* bodyIDPtr = (JPH::BodyID*)rb.RuntimeBody;

            if (!bodyIDPtr) {
                // Determine Layer and Motion Type
                JPH::ObjectLayer layer = (rb.MotionType == Rigidbody3DMotionType::Static) ? Layers::NON_MOVING : Layers::MOVING;
                JPH::EMotionType motionType = JPH::EMotionType::Static;
                if (rb.MotionType == Rigidbody3DMotionType::Kinematic) motionType = JPH::EMotionType::Kinematic;
                if (rb.MotionType == Rigidbody3DMotionType::Dynamic) motionType = JPH::EMotionType::Dynamic;

                // Create Shape
                JPH::ShapeRefC shape;
                if (world.HasComponent<Collider3DComponent>(entity)) {
                    auto& cc = world.GetComponent<Collider3DComponent>(entity);
                    if (cc.ShapeType == Collider3DShapeType::Box) {
                        shape = new JPH::BoxShape(JPH::Vec3(cc.BoxHalfExtents.x * tc.scale.x, cc.BoxHalfExtents.y * tc.scale.y, cc.BoxHalfExtents.z * tc.scale.z));
                    } else if (cc.ShapeType == Collider3DShapeType::Sphere) {
                        shape = new JPH::SphereShape(cc.SphereRadius * tc.scale.x);
                    } else if (cc.ShapeType == Collider3DShapeType::Capsule) {
                        shape = new JPH::CapsuleShape(cc.CapsuleHalfHeight * tc.scale.y, cc.CapsuleRadius * tc.scale.x);
                    } else if (cc.ShapeType == Collider3DShapeType::TriangleMesh || cc.ShapeType == Collider3DShapeType::ConvexHull) {
                        if (world.HasComponent<MeshComponent>(entity)) {
                            auto& mc = world.GetComponent<MeshComponent>(entity);
                            if (mc.MeshPtr) {
                                const auto& vertices = mc.MeshPtr->GetVertices();
                                const auto& indices = mc.MeshPtr->GetIndices();
                                
                                if (!vertices.empty() && !indices.empty()) {
                                    if (cc.ShapeType == Collider3DShapeType::TriangleMesh) {
                                        JPH::TriangleList triangles;
                                        triangles.reserve(indices.size() / 3);
                                        for (size_t i = 0; i < indices.size(); i += 3) {
                                            triangles.push_back(JPH::Triangle(
                                            JPH::Float3 { vertices[indices[i]].Position[0], vertices[indices[i]].Position[1], vertices[indices[i]].Position[2] },
                                            JPH::Float3 { vertices[indices[i+1]].Position[0], vertices[indices[i+1]].Position[1], vertices[indices[i+1]].Position[2] },
                                            JPH::Float3 { vertices[indices[i+2]].Position[0], vertices[indices[i+2]].Position[1], vertices[indices[i+2]].Position[2] }
                                        ));
                                        }
                                        JPH::MeshShapeSettings meshSettings(triangles);
                                        JPH::Shape::ShapeResult result = meshSettings.Create();
                                        if (result.IsValid()) shape = result.Get();
                                    } else { // ConvexHull
                                        JPH::Array<JPH::Vec3> points;
                                        points.reserve(vertices.size());
                                        for (const auto& v : vertices) {
                                        points.push_back(JPH::Vec3 { v.Position[0], v.Position[1], v.Position[2] });
                                    }
                                        JPH::ConvexHullShapeSettings hullSettings(points);
                                        JPH::Shape::ShapeResult result = hullSettings.Create();
                                        if (result.IsValid()) shape = result.Get();
                                    }
                                }
                            }
                        }
                    }
                }

                if (!shape) {
                    // Default fallback Box if no specific shape was created or mesh data was invalid
                    shape = new JPH::BoxShape(JPH::Vec3(0.5f, 0.5f, 0.5f));
                }

                // Initial Transform
                JPH::Vec3 position(tc.position.x, tc.position.y, tc.position.z);
                JPH::Quat rotation(tc.rotation.x, tc.rotation.y, tc.rotation.z, tc.rotation.w);

                JPH::BodyCreationSettings settings(shape, position, rotation, motionType, layer);
                settings.mFriction = rb.Friction;
                settings.mRestitution = rb.Restitution;
                settings.mLinearDamping = rb.LinearDamping;
                settings.mAngularDamping = rb.AngularDamping;
                settings.mAllowSleeping = rb.AllowSleeping;
                settings.mIsSensor = rb.Sensor;
                
                if (motionType == JPH::EMotionType::Dynamic) {
                    settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
                    settings.mMassPropertiesOverride.mMass = rb.Mass;
                }

                settings.mUserData = (JPH::uint64)entity.value;

                JPH::Body* body = bodyInterface.CreateBody(settings);
                if (body) {
                    JPH::BodyID* newBodyID = new JPH::BodyID(body->GetID());
                    rb.RuntimeBody = (void*)newBodyID;
                    bodyIDPtr = newBodyID;
                    // Note: In editor we shouldn't necessarily add to the simulated world yet
                    // But for simplicity of syncing, we add it, but don't step the world
                    bodyInterface.AddBody(*newBodyID, JPH::EActivation::Activate);
                }
            }

            // Sync Editor Changes to Physics Engine
            if (!isPlaying && bodyIDPtr) {
                JPH::Vec3 position(tc.position.x, tc.position.y, tc.position.z);
                JPH::Quat rotation(tc.rotation.x, tc.rotation.y, tc.rotation.z, tc.rotation.w);
                bodyInterface.SetPositionAndRotation(*bodyIDPtr, position, rotation, JPH::EActivation::DontActivate);
            }
        }

        // 2. Step Simulation
        if (isPlaying) {
            const int cCollisionSteps = 1;
            m_PhysicsSystem->Update(dt, cCollisionSteps, m_TempAllocator, m_JobSystem);

            // 3. Sync Physics Engine to ECS
            for (auto entity : entities) {
                if (!world.HasComponent<Rigidbody3DComponent>(entity) || !world.HasComponent<TransformComponent>(entity)) continue;

                auto& rb = world.GetComponent<Rigidbody3DComponent>(entity);
                auto& tc = world.GetComponent<TransformComponent>(entity);

                JPH::BodyID* bodyIDPtr = (JPH::BodyID*)rb.RuntimeBody;
                if (bodyIDPtr && rb.MotionType != Rigidbody3DMotionType::Static) {
                    JPH::Vec3 position = bodyInterface.GetPosition(*bodyIDPtr);
                    JPH::Quat rotation = bodyInterface.GetRotation(*bodyIDPtr);

                    tc.position = Math::Vec3f(position.GetX(), position.GetY(), position.GetZ());
                    tc.rotation = Math::Quatf(rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW());
                }
            }
        }
    }

    void Physics3DSystem::OnRigidbodyAdded(Entity entity, World& world) {
        // Implementation follows in next steps
    }

    void Physics3DSystem::OnRigidbodyRemoved(Entity entity, World& world) {
        // Implementation follows in next steps
    }

} // namespace ecs
} // namespace ge

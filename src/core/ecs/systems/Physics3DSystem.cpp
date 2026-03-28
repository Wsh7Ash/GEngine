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
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/SoftBody/SoftBody.h>
#include <Jolt/Physics/SoftBody/SoftBodyCreationSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodySharedSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodySurface.h>
#include <Jolt/Physics/SoftBody/SoftBodyVertex.h>
#include <Jolt/Physics/Constraints/Constraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/SpringSettings.h>

#include "Physics3DSystem.h"
#include "../World.h"
#include "../components/TransformComponent.h"
#include "../components/Rigidbody3DComponent.h"
#include "../components/CharacterController3DComponent.h"
#include "../components/Collider3DComponent.h"
#include "../components/MeshComponent.h"
#include "../components/SoftBodyComponent.h"
#include "../components/JointComponent.h"
#include "../components/InputStateComponent.h"
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

        // Initialize Soft Body Interface
        m_SoftBodyInterface = m_PhysicsSystem->GetSoftBodyInterface();
        
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
                    } else if (cc.ShapeType == Collider3DShapeType::HeightField) {
                        if (!cc.HeightField.Heights.empty() && cc.HeightField.Width > 0 && cc.HeightField.Depth > 0) {
                            JPH::HeightFieldShapeSettings settings(
                                cc.HeightField.Heights.data(),
                                cc.HeightField.Width,
                                cc.HeightField.Depth,
                                cc.HeightField.ScaleX,
                                cc.HeightField.ScaleZ,
                                cc.HeightField.HeightScale,
                                cc.HeightField.OffsetY,
                                JPH::Vec3(0, 1, 0)
                            );
                            JPH::Shape::ShapeResult result = settings.Create();
                            if (result.IsValid()) shape = result.Get();
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

        // 1.5 Update and Create Soft Bodies
        if (isPlaying && m_SoftBodyInterface) {
            auto softBodies = world.Query<SoftBodyComponent, MeshComponent, TransformComponent>();
            for (auto entity : softBodies) {
                auto& sbc = world.GetComponent<SoftBodyComponent>(entity);
                auto& mc = world.GetComponent<MeshComponent>(entity);
                auto& tc = world.GetComponent<TransformComponent>(entity);

                JPH::SoftBody* softBody = (JPH::SoftBody*)sbc.RuntimeSoftBody;

                if (!softBody && mc.MeshPtr) {
                    const auto& vertices = mc.MeshPtr->GetVertices();
                    const auto& indices = mc.MeshPtr->GetIndices();

                    if (vertices.empty() || indices.empty()) continue;

                    JPH::SoftBodySharedSettings* sharedSettings = new JPH::SoftBodySharedSettings();
                    
                    sharedSettings->mVertices.resize(vertices.size());
                    for (size_t i = 0; i < vertices.size(); ++i) {
                        JPH::SoftBodyVertex& v = sharedSettings->mVertices[i];
                        v.mPosition = JPH::Vec3(vertices[i].Position[0], vertices[i].Position[1], vertices[i].Position[2]);
                        v.mVelocity = JPH::Vec3::sZero();
                        v.mNormal = JPH::Vec3(vertices[i].Normal[0], vertices[i].Normal[1], vertices[i].Normal[2]);
                        v.mInvMass = sbc.Settings.Mass > 0.0f ? 1.0f / (sbc.Settings.Mass / vertices.size()) : 0.0f;
                        v.mCollisionRadius = 0.01f;
                    }

                    std::vector<uint32_t> faces;
                    faces.reserve(indices.size());
                    for (size_t i = 0; i < indices.size(); ++i) {
                        faces.push_back(indices[i]);
                    }

                    sharedSettings->SetFaces(faces);
                    sharedSettings->CalculateGrams();
                    sharedSettings->CalculateConstraints(sbc.Settings.Stiffness, sbc.Settings.Iterations);

                    JPH::SoftBodyCreationSettings settings(sharedSettings);
                    settings.mGravityFactor = sbc.Settings.GravityFactor;
                    settings.mPressure = sbc.Settings.Pressure;
                    
                    JPH::Vec3 position(tc.position.x, tc.position.y, tc.position.z);
                    settings.mPosition = position;

                    softBody = m_SoftBodyInterface->CreateBody(settings);
                    if (softBody) {
                        softBody->SetUserData((JPH::uint64)entity.value);
                        m_SoftBodyInterface->AddBody(softBody->GetID());
                        sbc.RuntimeSoftBody = softBody;
                    }
                }

                if (softBody && sbc.Settings.MotionType != SoftBodyMotionType::Static) {
                    JPH::Vec3 gravity = m_PhysicsSystem->GetGravity() * sbc.Settings.GravityFactor;
                    
                    if (sbc.Settings.WindFactor > 0.0f) {
                        JPH::Vec3 wind = JPH::Vec3(
                            sbc.Settings.WindDirection.x * sbc.Settings.WindFactor,
                            sbc.Settings.WindDirection.y * sbc.Settings.WindFactor,
                            sbc.Settings.WindDirection.z * sbc.Settings.WindFactor
                        );
                        gravity += wind;
                    }
                    
                    softBody->AddForce(gravity);
                    
                    if (sbc.Settings.Damping > 0.0f) {
                        softBody->SetLinearDamping(sbc.Settings.Damping);
                    }
                }
            }

            for (auto entity : softBodies) {
                auto& sbc = world.GetComponent<SoftBodyComponent>(entity);
                auto& mc = world.GetComponent<MeshComponent>(entity);

                JPH::SoftBody* softBody = (JPH::SoftBody*)sbc.RuntimeSoftBody;
                if (!softBody || !mc.MeshPtr) continue;

                auto& vertices = mc.MeshPtr->GetVertices();
                
                if (vertices.size() != softBody->GetVertices().size()) continue;

                for (size_t i = 0; i < softBody->GetVertices().size(); ++i) {
                    JPH::Vec3 pos = softBody->GetVertices()[i].mPosition;
                    JPH::Vec3 norm = softBody->GetVertices()[i].mNormal;
                    
                    vertices[i].Position[0] = pos.GetX();
                    vertices[i].Position[1] = pos.GetY();
                    vertices[i].Position[2] = pos.GetZ();
                    vertices[i].Normal[0] = norm.GetX();
                    vertices[i].Normal[1] = norm.GetY();
                    vertices[i].Normal[2] = norm.GetZ();
                }

                mc.MeshPtr->SetData(vertices.data(), (uint32_t)(vertices.size() * sizeof(renderer::Vertex)));
                sbc.VerticesDirty = true;
            }
        }

        // 1.75 Update and Create Character Controllers
        if (isPlaying) {
            auto characters = world.Query<CharacterController3DComponent, TransformComponent>();
            for (auto entity : characters) {
                auto& cc = world.GetComponent<CharacterController3DComponent>(entity);
                auto& tc = world.GetComponent<TransformComponent>(entity);

                if (!cc.RuntimeCharacter) {
                    JPH::CharacterVirtualSettings settings;
                    settings.mShape = new JPH::CapsuleShape(0.5f * cc.Height - cc.Radius, cc.Radius);
                    settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -0.5f * cc.Height);
                    settings.mUp = JPH::Vec3::sAxisY();
                    settings.mCharacterPadding = cc.CharacterPadding;
                    settings.mMaxSlopeAngle = cc.MaxSlopeAngle * JPH::JPH_PI / 180.0f;
                    settings.mMaxStrength = cc.MaxStrength;
                    settings.mMass = cc.Mass;

                    JPH::Vec3 position(tc.position.x, tc.position.y, tc.position.z);
                    JPH::Quat rotation(tc.rotation.x, tc.rotation.y, tc.rotation.z, tc.rotation.w);

                    JPH::CharacterVirtual* character = new JPH::CharacterVirtual(&settings, position, rotation, m_PhysicsSystem);
                    cc.RuntimeCharacter = character;
                }

                JPH::CharacterVirtual* character = cc.RuntimeCharacter;

                JPH::Vec3 gravity = m_PhysicsSystem->GetGravity();
                JPH::Vec3 linearVelocity = character->GetLinearVelocity();
                
                float speedMultiplier = 1.0f;
                bool hasInputState = world.HasComponent<InputStateComponent>(entity);
                
                if (hasInputState) {
                    auto& input = world.GetComponent<InputStateComponent>(entity);
                    
                    // Store previous state
                    input.PreviousState = input.CurrentState;
                    input.PreviousPosition = tc.position;
                    
                    // Process input state
                    Math::Vec3f moveDir = {input.MoveAxis.X, 0.0f, input.MoveAxis.Z};
                    float moveLength = Math::Length(moveDir);
                    
                    if (moveLength > 0.01f) {
                        moveDir = Math::Normalize(moveDir);
                        
                        if (input.Sprint.IsPressed) {
                            speedMultiplier = input.RunMultiplier;
                            input.CurrentState = MovementState::Running;
                        } else {
                            input.CurrentState = MovementState::Walking;
                        }
                        
                        linearVelocity.SetX(moveDir.x * input.MoveSpeed * speedMultiplier);
                        linearVelocity.SetZ(moveDir.z * input.MoveSpeed * speedMultiplier);
                    } else {
                        input.CurrentState = MovementState::Idle;
                        linearVelocity.SetX(0.0f);
                        linearVelocity.SetZ(0.0f);
                    }
                    
                    if (!character->IsSupported()) {
                        if (linearVelocity.GetY() > 0.0f) {
                            input.CurrentState = MovementState::Jumping;
                        } else {
                            input.CurrentState = MovementState::Falling;
                        }
                    }
                    
                    // Movement state changed callback
                    if (input.CurrentState != input.PreviousState) {
                        if (world.HasComponent<NativeScriptComponent>(entity)) {
                            auto& nsc = world.GetComponent<NativeScriptComponent>(entity);
                            if (nsc.instance) {
                                nsc.instance->OnMovementStateChanged(input.CurrentState, input.PreviousState);
                            }
                        }
                    }
                    
                    // Jump handling
                    if (input.Jump.IsJustPressed && character->IsSupported()) {
                        linearVelocity.SetY(input.Jump.Value);
                        if (world.HasComponent<NativeScriptComponent>(entity)) {
                            auto& nsc = world.GetComponent<NativeScriptComponent>(entity);
                            if (nsc.instance) nsc.instance->OnJumpPressed();
                        }
                    }
                } else {
                    // Legacy behavior - allow user script to override LinearVelocity horizontal components
                    linearVelocity.SetX(cc.LinearVelocity.x);
                    linearVelocity.SetZ(cc.LinearVelocity.z);
                }
                
                if (!character->IsSupported()) {
                    linearVelocity += gravity * dt;
                } else if (cc.LinearVelocity.y > 0.0f && !hasInputState) {
                    linearVelocity.SetY(cc.LinearVelocity.y); // Jump
                    cc.LinearVelocity.y = 0.0f; // Reset jump intent
                }

                character->SetLinearVelocity(linearVelocity);

                JPH::DefaultBroadPhaseLayerFilter broadPhaseLayerFilter = m_PhysicsSystem->GetDefaultBroadPhaseLayerFilter(Layers::MOVING);
                JPH::DefaultObjectLayerFilter objectLayerFilter = m_PhysicsSystem->GetDefaultLayerFilter(Layers::MOVING);
                JPH::BodyFilter bodyFilter;
                JPH::ShapeFilter shapeFilter;

                JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
                character->ExtendedUpdate(dt,
                    gravity,
                    updateSettings,
                    broadPhaseLayerFilter,
                    objectLayerFilter,
                    bodyFilter,
                    shapeFilter,
                    *m_TempAllocator);

                cc.IsGrounded = character->IsSupported();
                cc.LinearVelocity = Math::Vec3f(character->GetLinearVelocity().GetX(), character->GetLinearVelocity().GetY(), character->GetLinearVelocity().GetZ());

                JPH::Vec3 pos = character->GetPosition();
                Math::Vec3f newPos = Math::Vec3f(pos.GetX(), pos.GetY(), pos.GetZ());
                
                // Smooth interpolation to prevent jitter
                if (hasInputState) {
                    auto& input = world.GetComponent<InputStateComponent>(entity);
                    
                    // Store physics position for interpolation
                    input.InterpolatedPosition = newPos;
                    input.InterpolationAlpha = 0.0f;
                    
                    // Interpolate between previous and current position
                    Math::Vec3f interpolated = Math::Lerp(input.PreviousPosition, newPos, 0.5f);
                    tc.position = interpolated;
                    
                    // Call transform interpolation hook
                    if (world.HasComponent<NativeScriptComponent>(entity)) {
                        auto& nsc = world.GetComponent<NativeScriptComponent>(entity);
                        if (nsc.instance) {
                            nsc.instance->OnTransformInterpolate(interpolated, 0.5f);
                        }
                    }
                } else {
                    tc.position = newPos;
                }
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

        if (isPlaying) {
            auto joints = world.Query<JointComponent, TransformComponent>();
            for (auto entity : joints) {
                auto& jc = world.GetComponent<JointComponent>(entity);
                
                if (!jc.RuntimeJoint && jc.Settings.BodyA && jc.Settings.BodyB) {
                    if (!world.HasComponent<Rigidbody3DComponent>(jc.Settings.BodyA) ||
                        !world.HasComponent<Rigidbody3DComponent>(jc.Settings.BodyB)) {
                        continue;
                    }
                    
                    auto& rbA = world.GetComponent<Rigidbody3DComponent>(jc.Settings.BodyA);
                    auto& rbB = world.GetComponent<Rigidbody3DComponent>(jc.Settings.BodyB);
                    
                    JPH::BodyID* bodyIDA = (JPH::BodyID*)rbA.RuntimeBody;
                    JPH::BodyID* bodyIDB = (JPH::BodyID*)rbB.RuntimeBody;
                    
                    if (!bodyIDA || !bodyIDB) continue;
                    
                    JPH::Body* bodyA = bodyInterface.GetBody(*bodyIDA);
                    JPH::Body* bodyB = bodyInterface.GetBody(*bodyIDB);
                    
                    if (!bodyA || !bodyB) continue;
                    
                    JPH::Vec3 anchorA(jc.Settings.LocalAnchorA.x, jc.Settings.LocalAnchorA.y, jc.Settings.LocalAnchorA.z);
                    JPH::Vec3 anchorB(jc.Settings.LocalAnchorB.x, jc.Settings.LocalAnchorB.y, jc.Settings.LocalAnchorB.z);
                    
                    JPH::Constraint* constraint = nullptr;
                    
                    switch (jc.Settings.Type) {
                        case JointType::Point: {
                            JPH::PointConstraintSettings settings;
                            settings.mPoint = anchorA;
                            settings.mBody2 = bodyB;
                            settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
                            constraint = settings.Create(*bodyA, *bodyB);
                            break;
                        }
                        case JointType::Fixed: {
                            JPH::FixedConstraintSettings settings;
                            settings.mBody2 = bodyB;
                            settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
                            constraint = settings.Create(*bodyA, *bodyB);
                            break;
                        }
                        case JointType::Hinge: {
                            JPH::HingeConstraintSettings settings;
                            settings.mBody2 = bodyB;
                            settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
                            
                            JPH::Vec3 axisA(0, 1, 0);
                            if (jc.Settings.AxisA == JointAxis::X) axisA = JPH::Vec3(1, 0, 0);
                            else if (jc.Settings.AxisA == JointAxis::Z) axisA = JPH::Vec3(0, 0, 1);
                            
                            settings.mAxis1 = axisA;
                            settings.mAxis2 = axisA;
                            
                            if (jc.Settings.RotationLimits.Max > jc.Settings.RotationLimits.Min) {
                                settings.mLimitsMin = jc.Settings.RotationLimits.Min;
                                settings.mLimitsMax = jc.Settings.RotationLimits.Max;
                            }
                            
                            if (jc.Settings.Motor.Enabled) {
                                settings.mMotorSettings.mMode = JPH::EMotorMode::Velocity;
                                settings.mMotorSettings.mTargetVelocity = jc.Settings.Motor.TargetVelocity;
                                settings.mMotorSettings.mMaxForce = jc.Settings.Motor.MaxForce;
                            }
                            
                            constraint = settings.Create(*bodyA, *bodyB);
                            break;
                        }
                        case JointType::Slider: {
                            JPH::SliderConstraintSettings settings;
                            settings.mBody2 = bodyB;
                            settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
                            
                            JPH::Vec3 axis(0, 1, 0);
                            if (jc.Settings.AxisA == JointAxis::X) axis = JPH::Vec3(1, 0, 0);
                            else if (jc.Settings.AxisA == JointAxis::Z) axis = JPH::Vec3(0, 0, 1);
                            
                            settings.mAxis1 = axis;
                            settings.mAxis2 = axis;
                            
                            if (jc.Settings.PositionLimits.Max > jc.Settings.PositionLimits.Min) {
                                settings.mLimitsMin = jc.Settings.PositionLimits.Min;
                                settings.mLimitsMax = jc.Settings.PositionLimits.Max;
                            }
                            
                            constraint = settings.Create(*bodyA, *bodyB);
                            break;
                        }
                        case JointType::Distance: {
                            JPH::DistanceConstraintSettings settings;
                            settings.mBody2 = bodyB;
                            settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
                            settings.mPoint1 = anchorA;
                            settings.mPoint2 = anchorB;
                            constraint = settings.Create(*bodyA, *bodyB);
                            break;
                        }
                        case JointType::Spring: {
                            JPH::DistanceConstraintSettings settings;
                            settings.mBody2 = bodyB;
                            settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
                            settings.mPoint1 = anchorA;
                            settings.mPoint2 = anchorB;
                            settings.mIsSpring = true;
                            if (jc.Settings.LinearSpring.Enabled) {
                                settings.mFrequency = jc.Settings.LinearSpring.Frequency;
                                settings.mDamping = jc.Settings.LinearSpring.Damping;
                            }
                            constraint = settings.Create(*bodyA, *bodyB);
                            break;
                        }
                    }
                    
                    if (constraint) {
                        m_PhysicsSystem->AddConstraint(constraint);
                        jc.RuntimeJoint = constraint;
                        
                        if (jc.Settings.EnableCollision) {
                            constraint->SetCollisionEnabled(true);
                        }
                    }
                }
                
                if (jc.RuntimeJoint && jc.Settings.BreakForce > 0.0f) {
                    JPH::Constraint* constraint = (JPH::Constraint*)jc.RuntimeJoint;
                    float totalForce = 0.0f;
                    
                    constraint->CalcConstraintError(totalForce);
                    jc.CurrentStress = totalForce;
                    
                    if (totalForce > jc.Settings.BreakForce) {
                        jc.IsBroken = true;
                        m_PhysicsSystem->RemoveConstraint(constraint);
                        constraint->Release();
                        jc.RuntimeJoint = nullptr;
                        
                        if (jc.OnBreak) {
                            jc.OnBreak(jc.Settings.BodyA, jc.Settings.BodyB);
                        }
                    }
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

using System;
using System.Collections.Generic;

namespace GameEngine.SDK
{
    public class AnimatorComponent
    {
        public string CurrentState { get; set; }
        public float StateTime { get; set; }
        public int CurrentFrameIndex { get; set; }
    }
    public class AudioListenerComponent
    {
        public bool IsActive { get; set; }
    }
    public class AudioSourceComponent
    {
        public ulong ClipHandle { get; set; }
        public string FilePath { get; set; }
        public float Volume { get; set; }
        public float Pitch { get; set; }
        public bool Loop { get; set; }
        public bool PlayOnAwake { get; set; }
        public bool Is3D { get; set; }
        public float MinDistance { get; set; }
        public float MaxDistance { get; set; }
        public bool IsPlaying { get; set; }
        public bool HasStarted { get; set; }
    }
    public class BuoyancyComponent
    {
        public BuoyancyMode Mode { get; set; }
        public float FluidDensity { get; set; }
        public float LinearDrag { get; set; }
        public float AngularDrag { get; set; }
        public float FlowStrength { get; set; }
    }
    public class CanvasComponent
    {
        public RenderMode Mode { get; set; }
        public uint SortingOrder { get; set; }
    }
    public class CharacterController3DComponent
    {
        public float Height { get; set; }
        public float Radius { get; set; }
        public float Mass { get; set; }
        public float MaxSlopeAngle { get; set; }
        public float MaxStrength { get; set; }
        public float CharacterPadding { get; set; }
    }
    public class ClothComponent
    {
        public ClothCreationSettings Settings { get; set; }
    }
    public class Collider3DComponent
    {
        public Collider3DShapeType ShapeType { get; set; }
    }
    public class DecalComponent
    {
        public Mat4f Projection { get; set; }
        public float FadeStart { get; set; }
        public float FadeEnd { get; set; }
        public bool Enabled { get; set; }
    }
    public class IDComponent
    {
        public UUID ID { get; set; }
    }
    public class InputStateComponent
    {
        public InputMode CurrentMode { get; set; }
        public MovementState CurrentState { get; set; }
        public MovementState PreviousState { get; set; }
        public InputAxis MoveAxis { get; set; }
        public InputAxis LookAxis { get; set; }
        public InputAxis MouseDelta { get; set; }
        public InputAction Jump { get; set; }
        public InputAction Crouch { get; set; }
        public InputAction Sprint { get; set; }
        public InputAction Interact { get; set; }
        public InputAction Attack { get; set; }
        public InputAction AltAttack { get; set; }
        public InputAction Reload { get; set; }
        public InputAction Inventory { get; set; }
        public InputAction Pause { get; set; }
        public float MouseSensitivity { get; set; }
        public float GamepadSensitivity { get; set; }
        public bool IsLocked { get; set; }
        public float MoveSpeed { get; set; }
        public float RunMultiplier { get; set; }
        public float CrouchMultiplier { get; set; }
    }
    public class JointComponent
    {
        public JointCreationSettings Settings { get; set; }
        public bool IsBroken { get; set; }
        public float CurrentStress { get; set; }
    }
    public class DestructibleComponent
    {
        public DestructibleSettings Settings { get; set; }
        public bool IsFractured { get; set; }
    }
    public class LightComponent
    {
        public LightType Type { get; set; }
    }
    public class MeshComponent
    {
        public string MeshPath { get; set; }
        public float DistanceThreshold { get; set; }
    }
    public class ModelComponent
    {
        public AssetHandle ModelHandle { get; set; }
    }
    public class NativeScriptComponent
    {
        public string ScriptName { get; set; }
    }
    public class ParticleEmitterComponent
    {
        public ParticleProps Props { get; set; }
        public float EmissionRate { get; set; }
        public bool IsEmitting { get; set; }
        public float EmissionAccumulator { get; set; }
    }
    public class PostProcessComponent
    {
        public bool Enabled { get; set; }
        public bool BloomEnabled { get; set; }
        public float BloomIntensity { get; set; }
        public float BloomThreshold { get; set; }
        public float Exposure { get; set; }
        public float Gamma { get; set; }
        public bool VolumetricFogEnabled { get; set; }
        public float FogDensity { get; set; }
        public float FogHeight { get; set; }
        public float FogHeightFalloff { get; set; }
        public float FogAnisotropy { get; set; }
        public float FogMultiScattering { get; set; }
    }
    public class RelationshipComponent
    {
        public Entity Parent { get; set; }
    }
    public class Rigidbody2DComponent
    {
        public RigidBody2DType Type { get; set; }
        public bool FixedRotation { get; set; }
    }
    public class Rigidbody3DComponent
    {
        public Rigidbody3DMotionType MotionType { get; set; }
        public float Mass { get; set; }
        public float Friction { get; set; }
        public float Restitution { get; set; }
        public float LinearDamping { get; set; }
        public float AngularDamping { get; set; }
        public bool AllowSleeping { get; set; }
        public bool Sensor { get; set; }
        public bool CCD { get; set; }
        public int CollisionLayer { get; set; }
        public int CollisionMask { get; set; }
    }
    public class SkyboxComponent
    {
        public float Intensity { get; set; }
    }
    public class SoftBodyComponent
    {
        public SoftBodyCreationSettings Settings { get; set; }
        public string MeshPath { get; set; }
        public bool IsVisible { get; set; }
    }
    public class TagComponent
    {
        public string Tag { get; set; }
    }
    public class TerrainComponent
    {
        public uint GridWidth { get; set; }
        public uint GridDepth { get; set; }
        public float WorldSizeX { get; set; }
        public float WorldSizeZ { get; set; }
        public float HeightScale { get; set; }
        public AABB TotalBounds { get; set; }
        public uint ChunkSize { get; set; }
        public uint LODLevels { get; set; }
        public TerrainLOD CurrentLOD { get; set; }
        public string HeightmapPath { get; set; }
        public string MaterialPath { get; set; }
        public float MinHeight { get; set; }
        public float MaxHeight { get; set; }
        public bool CastShadows { get; set; }
        public bool ReceiveShadows { get; set; }
        public bool IsNavigationBuilt { get; set; }
        public bool IsPhysicsBuilt { get; set; }
    }
    public class TransformComponent
    {
        public Quatf Rotation { get; set; }
    }
    public class UIButtonComponent
    {
        public ButtonState State { get; set; }
    }
    public class VehicleComponent
    {
        public EngineSettings Engine { get; set; }
        public TransmissionSettings Transmission { get; set; }
        public BrakeSettings Brakes { get; set; }
        public float ChassisMass { get; set; }
    }
}

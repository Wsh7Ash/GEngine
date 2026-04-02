using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;

namespace GameEngine.SDK
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector3
    {
        public float X, Y, Z;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector3(float x, float y, float z)
        {
            X = x; Y = y; Z = z;
        }

        public static Vector3 Zero => new Vector3(0, 0, 0);
        public static Vector3 One => new Vector3(1, 1, 1);
        public static Vector3 Up => new Vector3(0, 1, 0);
        public static Vector3 Down => new Vector3(0, -1, 0);
        public static Vector3 Forward => new Vector3(0, 0, -1);
        public static Vector3 Back => new Vector3(0, 0, 1);
        public static Vector3 Left => new Vector3(-1, 0, 0);
        public static Vector3 Right => new Vector3(1, 0, 0);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public float MagnitudeSquared => X * X + Y * Y + Z * Z;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public float Magnitude => (float)Math.Sqrt(MagnitudeSquared);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector3 Normalized
        {
            get
            {
                float mag = Magnitude;
                return mag > 0 ? new Vector3(X / mag, Y / mag, Z / mag) : Zero;
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3 operator +(Vector3 a, Vector3 b) => new Vector3(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
        
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3 operator -(Vector3 a, Vector3 b) => new Vector3(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
        
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3 operator *(Vector3 a, float s) => new Vector3(a.X * s, a.Y * s, a.Z * s);
        
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3 operator /(Vector3 a, float s) => new Vector3(a.X / s, a.Y / s, a.Z / s);

        public override string ToString() => $"({X:F2}, {Y:F2}, {Z:F2})";
    }
    
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector2
    {
        public float X, Y;
        
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector2(float x, float y)
        {
            X = x; Y = y;
        }
        
        public static Vector2 Zero => new Vector2(0, 0);
        public static Vector2 One => new Vector2(1, 1);
        
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public float MagnitudeSquared => X * X + Y * Y;
        
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public float Magnitude => (float)Math.Sqrt(MagnitudeSquared);
        
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector2 Normalized
        {
            get
            {
                float mag = Magnitude;
                return mag > 0 ? new Vector2(X / mag, Y / mag) : Zero;
            }
        }
        
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2 operator +(Vector2 a, Vector2 b) => new Vector2(a.X + b.X, a.Y + b.Y);
        
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2 operator -(Vector2 a, Vector2 b) => new Vector2(a.X - b.X, a.Y - b.Y);
        
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2 operator *(Vector2 a, float s) => new Vector2(a.X * s, a.Y * s);
        
        public override string ToString() => $"({X:F2}, {Y:F2})";
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Vector4
    {
        public float X, Y, Z, W;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector4(float x, float y, float z, float w)
        {
            X = x; Y = y; Z = z; W = w;
        }

        public static Vector4 Zero => new Vector4(0, 0, 0, 0);
    }

    public interface IComponentWrapper
    {
        IntPtr NativePtr { get; }
        void SetNativePtr(IntPtr ptr);
        bool IsValid { get; }
    }

    public class TransformComponent : IComponentWrapper
    {
        private IntPtr _nativePtr;
        
        public IntPtr NativePtr => _nativePtr;
        
        public bool IsValid => _nativePtr != IntPtr.Zero;

        public void SetNativePtr(IntPtr ptr)
        {
            _nativePtr = ptr;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct NativeTransform
        {
            public Vector3 position;
            public Quaternion rotation;
            public Vector3 scale;
        }

        public Vector3 Position
        {
            get
            {
                if (!IsValid) return Vector3.Zero;
                unsafe
                {
                    return ((NativeTransform*)_nativePtr)->position;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeTransform*)_nativePtr)->position = value;
                }
            }
        }

        public Quaternion Rotation
        {
            get
            {
                if (!IsValid) return Quaternion.Identity;
                unsafe
                {
                    return ((NativeTransform*)_nativePtr)->rotation;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeTransform*)_nativePtr)->rotation = value;
                }
            }
        }

        public Vector3 Scale
        {
            get
            {
                if (!IsValid) return Vector3.One;
                unsafe
                {
                    return ((NativeTransform*)_nativePtr)->scale;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeTransform*)_nativePtr)->scale = value;
                }
            }
        }

        public Vector3 Forward => Rotation.Normalized * Vector3.Forward;
        public Vector3 Back => Rotation.Normalized * Vector3.Back;
        public Vector3 Up => Rotation.Normalized * Vector3.Up;
        public Vector3 Down => Rotation.Normalized * Vector3.Down;
        public Vector3 Left => Rotation.Normalized * Vector3.Left;
        public Vector3 Right => Rotation.Normalized * Vector3.Right;

        public Vector3 EulerAngles
        {
            get
            {
                Quaternion q = Rotation;
                float sinr_cosp = 2 * (q.W * q.X + q.Y * q.Z);
                float cosr_cosp = 1 - 2 * (q.X * q.X + q.Y * q.Y);
                float roll = (float)Math.Atan2(sinr_cosp, cosr_cosp);

                float sinp = 2 * (q.W * q.Y - q.Z * q.X);
                float pitch = Math.Abs(sinp) >= 1 ? (float)Math.CopySign(Math.PI / 2, sinp) : (float)Math.Asin(sinp);

                float siny_cosp = 2 * (q.W * q.Z + q.X * q.Y);
                float cosy_cosp = 1 - 2 * (q.Y * q.Y + q.Z * q.Z);
                float yaw = (float)Math.Atan2(siny_cosp, cosy_cosp);

                return new Vector3(roll, pitch, yaw);
            }
            set
            {
                Rotation = Quaternion.FromEuler(value.X, value.Y, value.Z);
            }
        }

        public Vector3 LocalPosition
        {
            get => Position;
            set => Position = value;
        }
    }

    public enum Rigidbody3DMotionType
    {
        Static = 0,
        Kinematic,
        Dynamic
    }

    public class Rigidbody3DComponent : IComponentWrapper
    {
        private IntPtr _nativePtr;
        
        public IntPtr NativePtr => _nativePtr;
        
        public bool IsValid => _nativePtr != IntPtr.Zero;

        public void SetNativePtr(IntPtr ptr)
        {
            _nativePtr = ptr;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct NativeRigidbody3D
        {
            public int MotionType;
            public float Mass;
            public float Friction;
            public float Restitution;
            public float LinearDamping;
            public float AngularDamping;
            public int AllowSleeping;
            public int Sensor;
            public int CCD;
            public int CollisionLayer;
            public IntPtr RuntimeBody;
        }

        public Rigidbody3DMotionType MotionType
        {
            get
            {
                if (!IsValid) return Rigidbody3DMotionType.Dynamic;
                unsafe
                {
                    return (Rigidbody3DMotionType)((NativeRigidbody3D*)_nativePtr)->MotionType;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeRigidbody3D*)_nativePtr)->MotionType = (int)value;
                }
            }
        }

        public float Mass
        {
            get
            {
                if (!IsValid) return 1.0f;
                unsafe
                {
                    return ((NativeRigidbody3D*)_nativePtr)->Mass;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeRigidbody3D*)_nativePtr)->Mass = value;
                }
            }
        }

        public float Friction
        {
            get
            {
                if (!IsValid) return 0.5f;
                unsafe
                {
                    return ((NativeRigidbody3D*)_nativePtr)->Friction;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeRigidbody3D*)_nativePtr)->Friction = value;
                }
            }
        }

        public float Restitution
        {
            get
            {
                if (!IsValid) return 0.0f;
                unsafe
                {
                    return ((NativeRigidbody3D*)_nativePtr)->Restitution;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeRigidbody3D*)_nativePtr)->Restitution = value;
                }
            }
        }

        public float LinearDamping
        {
            get
            {
                if (!IsValid) return 0.05f;
                unsafe
                {
                    return ((NativeRigidbody3D*)_nativePtr)->LinearDamping;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeRigidbody3D*)_nativePtr)->LinearDamping = value;
                }
            }
        }

        public float AngularDamping
        {
            get
            {
                if (!IsValid) return 0.05f;
                unsafe
                {
                    return ((NativeRigidbody3D*)_nativePtr)->AngularDamping;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeRigidbody3D*)_nativePtr)->AngularDamping = value;
                }
            }
        }

        public bool AllowSleeping
        {
            get
            {
                if (!IsValid) return true;
                unsafe
                {
                    return ((NativeRigidbody3D*)_nativePtr)->AllowSleeping != 0;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeRigidbody3D*)_nativePtr)->AllowSleeping = value ? 1 : 0;
                }
            }
        }

        public bool IsSensor
        {
            get
            {
                if (!IsValid) return false;
                unsafe
                {
                    return ((NativeRigidbody3D*)_nativePtr)->Sensor != 0;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeRigidbody3D*)_nativePtr)->Sensor = value ? 1 : 0;
                }
            }
        }

        public bool CCD
        {
            get
            {
                if (!IsValid) return false;
                unsafe
                {
                    return ((NativeRigidbody3D*)_nativePtr)->CCD != 0;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeRigidbody3D*)_nativePtr)->CCD = value ? 1 : 0;
                }
            }
        }

        public int CollisionLayer
        {
            get
            {
                if (!IsValid) return 1;
                unsafe
                {
                    return ((NativeRigidbody3D*)_nativePtr)->CollisionLayer;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeRigidbody3D*)_nativePtr)->CollisionLayer = value;
                }
            }
        }

        public int CollisionMask
        {
            get
            {
                if (!IsValid) return 1;
                unsafe
                {
                    return ((NativeRigidbody3D*)_nativePtr)->CollisionMask;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeRigidbody3D*)_nativePtr)->CollisionMask = value;
                }
            }
        }
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void Rigidbody3D_SetVelocity(ulong entityId, float x, float y, float z);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void Rigidbody3D_GetVelocity(ulong entityId, out float x, out float y, out float z);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void Rigidbody3D_AddForce(ulong entityId, float x, float y, float z);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void Rigidbody3D_AddImpulse(ulong entityId, float x, float y, float z);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void Rigidbody3D_AddTorque(ulong entityId, float x, float y, float z);
        
        private ulong _entityId = 0;
        
        public Vector3 Velocity
        {
            get
            {
                if (!IsValid || _entityId == 0) return Vector3.Zero;
                Rigidbody3D_GetVelocity(_entityId, out float x, out float y, out float z);
                return new Vector3(x, y, z);
            }
            set
            {
                if (!IsValid || _entityId == 0) return;
                Rigidbody3D_SetVelocity(_entityId, value.X, value.Y, value.Z);
            }
        }
        
        public void SetVelocity(float x, float y, float z)
        {
            if (!IsValid || _entityId == 0) return;
            Rigidbody3D_SetVelocity(_entityId, x, y, z);
        }
        
        public void AddForce(float x, float y, float z)
        {
            if (!IsValid || _entityId == 0) return;
            Rigidbody3D_AddForce(_entityId, x, y, z);
        }
        
        public void AddForce(Vector3 force) => AddForce(force.X, force.Y, force.Z);
        
        public void AddImpulse(float x, float y, float z)
        {
            if (!IsValid || _entityId == 0) return;
            Rigidbody3D_AddImpulse(_entityId, x, y, z);
        }
        
        public void AddImpulse(Vector3 impulse) => AddImpulse(impulse.X, impulse.Y, impulse.Z);
        
        public void AddTorque(float x, float y, float z)
        {
            if (!IsValid || _entityId == 0) return;
            Rigidbody3D_AddTorque(_entityId, x, y, z);
        }
        
        public void AddTorque(Vector3 torque) => AddTorque(torque.X, torque.Y, torque.Z);
        
        public void SetEntityId(ulong entityId) => _entityId = entityId;
    }

    public enum LightType
    {
        Directional = 0,
        Point,
        Spot,
        Area
    }

    public class LightComponent : IComponentWrapper
    {
        private IntPtr _nativePtr;
        
        public IntPtr NativePtr => _nativePtr;
        
        public bool IsValid => _nativePtr != IntPtr.Zero;

        public void SetNativePtr(IntPtr ptr)
        {
            _nativePtr = ptr;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct NativeLight
        {
            public int Type;
            public Vector3 Color;
            public float Intensity;
            public float Range;
            public Vector3 Direction;
            public float InnerConeAngle;
            public float OuterConeAngle;
            public int CastShadows;
            public int Enabled;
        }

        public LightType Type
        {
            get
            {
                if (!IsValid) return LightType.Point;
                unsafe
                {
                    return (LightType)((NativeLight*)_nativePtr)->Type;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeLight*)_nativePtr)->Type = (int)value;
                }
            }
        }

        public Vector3 Color
        {
            get
            {
                if (!IsValid) return Vector3.One;
                unsafe
                {
                    return ((NativeLight*)_nativePtr)->Color;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeLight*)_nativePtr)->Color = value;
                }
            }
        }

        public float Intensity
        {
            get
            {
                if (!IsValid) return 1.0f;
                unsafe
                {
                    return ((NativeLight*)_nativePtr)->Intensity;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeLight*)_nativePtr)->Intensity = value;
                }
            }
        }

        public float Range
        {
            get
            {
                if (!IsValid) return 10.0f;
                unsafe
                {
                    return ((NativeLight*)_nativePtr)->Range;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeLight*)_nativePtr)->Range = value;
                }
            }
        }

        public Vector3 Direction
        {
            get
            {
                if (!IsValid) return Vector3.Back;
                unsafe
                {
                    return ((NativeLight*)_nativePtr)->Direction;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeLight*)_nativePtr)->Direction = value;
                }
            }
        }

        public float InnerConeAngle
        {
            get
            {
                if (!IsValid) return 0.0f;
                unsafe
                {
                    return ((NativeLight*)_nativePtr)->InnerConeAngle;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeLight*)_nativePtr)->InnerConeAngle = value;
                }
            }
        }

        public float OuterConeAngle
        {
            get
            {
                if (!IsValid) return (float)(Math.PI / 4);
                unsafe
                {
                    return ((NativeLight*)_nativePtr)->OuterConeAngle;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeLight*)_nativePtr)->OuterConeAngle = value;
                }
            }
        }

        public bool CastShadows
        {
            get
            {
                if (!IsValid) return false;
                unsafe
                {
                    return ((NativeLight*)_nativePtr)->CastShadows != 0;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeLight*)_nativePtr)->CastShadows = value ? 1 : 0;
                }
            }
        }

        public bool Enabled
        {
            get
            {
                if (!IsValid) return true;
                unsafe
                {
                    return ((NativeLight*)_nativePtr)->Enabled != 0;
                }
            }
            set
            {
                if (!IsValid) return;
                unsafe
                {
                    ((NativeLight*)_nativePtr)->Enabled = value ? 1 : 0;
                }
            }
        }
    }
    
    public class CameraComponent : IComponentWrapper
    {
        private IntPtr _nativePtr;
        
        public IntPtr NativePtr => _nativePtr;
        
        public bool IsValid => _nativePtr != IntPtr.Zero;
        
        public void SetNativePtr(IntPtr ptr) => _nativePtr = ptr;
        
        [StructLayout(LayoutKind.Sequential)]
        private struct NativeCamera
        {
            public Vector3 Position;
            public Vector3 Forward;
            public Vector3 Up;
            public float FOV;
            public float AspectRatio;
            public float NearClip;
            public float FarClip;
        }
        
        public Vector3 Position
        {
            get
            {
                if (!IsValid) return Vector3.Zero;
                unsafe { return ((NativeCamera*)_nativePtr)->Position; }
            }
        }
        
        public Vector3 Forward
        {
            get
            {
                if (!IsValid) return Vector3.Forward;
                unsafe { return ((NativeCamera*)_nativePtr)->Forward; }
            }
        }
        
        public Vector3 Up
        {
            get
            {
                if (!IsValid) return Vector3.Up;
                unsafe { return ((NativeCamera*)_nativePtr)->Up; }
            }
        }
        
        public float FOV
        {
            get
            {
                if (!IsValid) return 60.0f;
                unsafe { return ((NativeCamera*)_nativePtr)->FOV; }
            }
            set
            {
                if (!IsValid) return;
                unsafe { ((NativeCamera*)_nativePtr)->FOV = value; }
            }
        }
        
        public float AspectRatio
        {
            get
            {
                if (!IsValid) return 1.77f;
                unsafe { return ((NativeCamera*)_nativePtr)->AspectRatio; }
            }
        }
        
        public float NearClip
        {
            get
            {
                if (!IsValid) return 0.1f;
                unsafe { return ((NativeCamera*)_nativePtr)->NearClip; }
            }
        }
        
        public float FarClip
        {
            get
            {
                if (!IsValid) return 1000.0f;
                unsafe { return ((NativeCamera*)_nativePtr)->FarClip; }
            }
        }
    }
    
    public class AudioSourceComponent : IComponentWrapper
    {
        private IntPtr _nativePtr;
        
        public IntPtr NativePtr => _nativePtr;
        
        public bool IsValid => _nativePtr != IntPtr.Zero;
        
        public void SetNativePtr(IntPtr ptr) => _nativePtr = ptr;
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void AudioSource_Play(ulong entityId);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void AudioSource_Stop(ulong entityId);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void AudioSource_Pause(ulong entityId);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool AudioSource_IsPlaying(ulong entityId);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void AudioSource_SetVolume(ulong entityId, float volume);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void AudioSource_SetPitch(ulong entityId, float pitch);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void AudioSource_SetLoop(ulong entityId, bool loop);
        
        private ulong _entityId = 0;
        
        public void Play()
        {
            if (_entityId != 0) AudioSource_Play(_entityId);
        }
        
        public void Stop()
        {
            if (_entityId != 0) AudioSource_Stop(_entityId);
        }
        
        public void Pause()
        {
            if (_entityId != 0) AudioSource_Pause(_entityId);
        }
        
        public bool IsPlaying
        {
            get => _entityId != 0 && AudioSource_IsPlaying(_entityId);
        }
        
        public float Volume
        {
            set { if (_entityId != 0) AudioSource_SetVolume(_entityId, value); }
        }
        
        public float Pitch
        {
            set { if (_entityId != 0) AudioSource_SetPitch(_entityId, value); }
        }
        
        public bool Loop
        {
            set { if (_entityId != 0) AudioSource_SetLoop(_entityId, value); }
        }
        
        public void SetEntityId(ulong entityId) => _entityId = entityId;
    }
}
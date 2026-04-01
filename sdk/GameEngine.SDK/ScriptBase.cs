using System;

namespace GameEngine.SDK
{
    public abstract class ScriptableEntity
    {
        protected Entity entity;
        
        public virtual void OnCreate() { }
        public virtual void OnUpdate(float deltaTime) { }
        public virtual void OnDestroy() { }
        
        public virtual void OnCollisionEnter(Entity other) { }
        public virtual void OnCollisionExit(Entity other) { }
        public virtual void OnTriggerEnter(Entity other) { }
        public virtual void OnTriggerExit(Entity other) { }
        
        protected T GetComponent<T>() where T : new() => entity.GetComponent<T>();
        protected bool HasComponent<T>() where T : new() => entity.HasComponent<T>();
        protected void AddComponent<T>() where T : new() => entity.AddComponent<T>();
        protected void RemoveComponent<T>() => entity.RemoveComponent<T>();
        
        protected void Log(string message) => Interop.LogInfo(message);
        protected void LogWarning(string message) => Interop.LogWarning(message);
        protected void LogError(string message) => Interop.LogError(message);
        
        protected bool IsKeyPressed(int keyCode) => Interop.IsKeyPressed(keyCode);
        protected bool IsMouseButtonPressed(int button) => Interop.IsMouseButtonPressed(button);
        
        public static Entity CreateEntity() => Interop.CreateEntity();
        
        public void Destroy() => entity.Destroy();
        
        public Vector3 Position
        {
            get => GetComponent<TransformComponent>().Position;
            set => GetComponent<TransformComponent>().Position = value;
        }
        
        public Vector3 Rotation
        {
            get => GetComponent<TransformComponent>().Rotation;
            set => GetComponent<TransformComponent>().Rotation = value;
        }
        
        public Vector3 Scale
        {
            get => GetComponent<TransformComponent>().Scale;
            set => GetComponent<TransformComponent>().Scale = value;
        }
    }
    
    public class TransformComponent
    {
        public Vector3 Position { get; set; }
        public Vector3 Rotation { get; set; }
        public Vector3 Scale { get; set; }
        
        public TransformComponent()
        {
            Position = new Vector3(0, 0, 0);
            Rotation = new Vector3(0, 0, 0);
            Scale = new Vector3(1, 1, 1);
        }
    }
    
    public struct Vector3
    {
        public float X, Y, Z;
        
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
        
        public float Magnitude => (float)Math.Sqrt(X * X + Y * Y + Z * Z);
        
        public Vector3 Normalized
        {
            get
            {
                float mag = Magnitude;
                return mag > 0 ? new Vector3(X / mag, Y / mag, Z / mag) : Zero;
            }
        }
        
        public static Vector3 operator +(Vector3 a, Vector3 b) => new Vector3(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
        public static Vector3 operator -(Vector3 a, Vector3 b) => new Vector3(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
        public static Vector3 operator *(Vector3 a, float s) => new Vector3(a.X * s, a.Y * s, a.Z * s);
        public static Vector3 operator /(Vector3 a, float s) => new Vector3(a.X / s, a.Y / s, a.Z / s);
    }
}
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
        
        protected T GetComponent<T>() where T : IComponentWrapper, new() => entity.GetComponent<T>();
        protected bool HasComponent<T>() where T : IComponentWrapper, new() => entity.HasComponent<T>();
        protected void AddComponent<T>() where T : IComponentWrapper, new() => entity.AddComponent<T>();
        protected void RemoveComponent<T>() where T : IComponentWrapper => entity.RemoveComponent<T>();
        
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
        
        public Quaternion Rotation
        {
            get => GetComponent<TransformComponent>().Rotation;
            set => GetComponent<TransformComponent>().Rotation = value;
        }
        
        public Vector3 Scale
        {
            get => GetComponent<TransformComponent>().Scale;
            set => GetComponent<TransformComponent>().Scale = value;
        }

        public Vector3 Forward => GetComponent<TransformComponent>().Forward;
        public Vector3 Back => GetComponent<TransformComponent>().Back;
        public Vector3 Up => GetComponent<TransformComponent>().Up;
        public Vector3 Down => GetComponent<TransformComponent>().Down;
        public Vector3 Left => GetComponent<TransformComponent>().Left;
        public Vector3 Right => GetComponent<TransformComponent>().Right;
    }
}
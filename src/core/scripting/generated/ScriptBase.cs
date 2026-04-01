using System;

namespace GameEngine.SDK
{
    public abstract class ScriptableEntity
    {
        protected Entity entity;
        
        public virtual void OnCreate() { }
        public virtual void OnUpdate(float deltaTime) { }
        public virtual void OnDestroy() { }
        
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
    }
    
    public abstract class Script
    {
        protected ScriptableEntity entity;
        
        internal void SetEntity(ScriptableEntity e) => entity = e;
        
        public virtual void OnCreate() { }
        public virtual void OnUpdate(float deltaTime) { }
        public virtual void OnDestroy() { }
    }
}

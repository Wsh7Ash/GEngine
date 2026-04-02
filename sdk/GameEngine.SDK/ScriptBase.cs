using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace GameEngine.SDK
{
    public abstract class ScriptableEntity
    {
        private Entity _entity;
        private bool _isInitialized = false;
        
        public Entity Entity => _entity;
        
        public virtual void OnCreate() { }
        public virtual void OnUpdate(float deltaTime) { }
        public virtual void OnDestroy() { }
        
        public virtual void OnCollisionEnter(Entity other) { }
        public virtual void OnCollisionExit(Entity other) { }
        public virtual void OnTriggerEnter(Entity other) { }
        public virtual void OnTriggerExit(Entity other) { }
        
        public virtual void OnTransformInterpolate(Vector3 interpolatedPos, float alpha) { }
        
        internal void SetEntity(Entity entity)
        {
            _entity = entity;
            _isInitialized = true;
            OnCreate();
        }
        
        public bool IsValid => _isInitialized && _entity.IsValid;
        
        protected T GetComponent<T>() where T : IComponentWrapper, new() => _entity.GetComponent<T>();
        protected bool HasComponent<T>() where T : IComponentWrapper, new() => _entity.HasComponent<T>();
        protected void AddComponent<T>() where T : IComponentWrapper, new() => _entity.AddComponent<T>();
        protected void RemoveComponent<T>() where T : IComponentWrapper => _entity.RemoveComponent<T>();
        
        protected void Log(string message) => Interop.LogInfo(message);
        protected void LogWarning(string message) => Interop.LogWarning(message);
        protected void LogError(string message) => Interop.LogError(message);
        
        protected bool IsKeyPressed(int keyCode) => Interop.IsKeyPressed(keyCode);
        protected bool IsMouseButtonPressed(int button) => Interop.IsMouseButtonPressed(button);
        
        public static Entity CreateEntity() => Interop.CreateEntity();
        
        public void Destroy() => _entity.Destroy();
        
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
        
        public Vector3 EulerAngles
        {
            get => GetComponent<TransformComponent>().EulerAngles;
            set => GetComponent<TransformComponent>().EulerAngles = value;
        }
    }
    
    public class ScriptManager
    {
        private static Dictionary<ulong, ScriptableEntity> _scripts = new Dictionary<ulong, ScriptableEntity>();
        private static Dictionary<Type, Func<ScriptableEntity>> _scriptFactories = new Dictionary<Type, Func<ScriptableEntity>>();
        
        public static void RegisterScript<T>() where T : ScriptableEntity, new()
        {
            _scriptFactories[typeof(T)] = () => new T();
        }
        
        public static ScriptableEntity CreateScript(ulong entityId, Type scriptType)
        {
            if (!_scriptFactories.ContainsKey(scriptType))
            {
                Interop.LogError($"Script type {scriptType.Name} not registered. Call RegisterScript<{scriptType.Name}>() first.");
                return null;
            }
            
            var script = _scriptFactories[scriptType]();
            script.SetEntity(new Entity(entityId));
            
            _scripts[entityId] = script;
            return script;
        }
        
        public static ScriptableEntity GetScript(ulong entityId)
        {
            return _scripts.ContainsKey(entityId) ? _scripts[entityId] : null;
        }
        
        public static void RemoveScript(ulong entityId)
        {
            if (_scripts.ContainsKey(entityId))
            {
                _scripts[entityId].OnDestroy();
                _scripts.Remove(entityId);
            }
        }
        
        public static void OnUpdate(float deltaTime)
        {
            foreach (var script in _scripts.Values)
            {
                script.OnUpdate(deltaTime);
            }
        }
        
        public static void OnCollisionEnter(ulong entityId, ulong otherId)
        {
            if (_scripts.ContainsKey(entityId))
            {
                _scripts[entityId].OnCollisionEnter(new Entity(otherId));
            }
        }
        
        public static void OnCollisionExit(ulong entityId, ulong otherId)
        {
            if (_scripts.ContainsKey(entityId))
            {
                _scripts[entityId].OnCollisionExit(new Entity(otherId));
            }
        }
        
        public static void OnTriggerEnter(ulong entityId, ulong otherId)
        {
            if (_scripts.ContainsKey(entityId))
            {
                _scripts[entityId].OnTriggerEnter(new Entity(otherId));
            }
        }
        
        public static void OnTriggerExit(ulong entityId, ulong otherId)
        {
            if (_scripts.ContainsKey(entityId))
            {
                _scripts[entityId].OnTriggerExit(new Entity(otherId));
            }
        }
        
        public static void OnTransformInterpolate(ulong entityId, Vector3 pos, float alpha)
        {
            if (_scripts.ContainsKey(entityId))
            {
                _scripts[entityId].OnTransformInterpolate(pos, alpha);
            }
        }
    }
    
    public class ScriptHost
    {
        [UnmanagedCallersOnly(EntryPoint = "ScriptManager_OnCreate", CallConventions = CallConventions.Cdecl)]
        public static void OnCreate(ulong entityId, [MarshalAs(UnmanagedType.LPStr)] string scriptTypeName)
        {
            var scriptType = Type.GetType(scriptTypeName);
            if (scriptType == null)
            {
                Interop.LogError($"Could not find script type: {scriptTypeName}");
                return;
            }
            
            var createMethod = typeof(ScriptManager).GetMethod("CreateScript");
            var genericMethod = createMethod.MakeGenericMethod(scriptType);
            genericMethod.Invoke(null, new object[] { entityId, scriptType });
        }
        
        [UnmanagedCallersOnly(EntryPoint = "ScriptManager_OnUpdate", CallConventions = CallConventions.Cdecl)]
        public static void OnUpdate(float deltaTime)
        {
            ScriptManager.OnUpdate(deltaTime);
        }
        
        [UnmanagedCallersOnly(EntryPoint = "ScriptManager_OnDestroy", CallConventions = CallConventions.Cdecl)]
        public static void OnDestroy(ulong entityId)
        {
            ScriptManager.RemoveScript(entityId);
        }
        
        [UnmanagedCallersOnly(EntryPoint = "ScriptManager_OnCollisionEnter", CallConventions = CallConventions.Cdecl)]
        public static void OnCollisionEnter(ulong entityId, ulong otherId)
        {
            ScriptManager.OnCollisionEnter(entityId, otherId);
        }
        
        [UnmanagedCallersOnly(EntryPoint = "ScriptManager_OnCollisionExit", CallConventions = CallConventions.Cdecl)]
        public static void OnCollisionExit(ulong entityId, ulong otherId)
        {
            ScriptManager.OnCollisionExit(entityId, otherId);
        }
        
        [UnmanagedCallersOnly(EntryPoint = "ScriptManager_OnTriggerEnter", CallConventions = CallConventions.Cdecl)]
        public static void OnTriggerEnter(ulong entityId, ulong otherId)
        {
            ScriptManager.OnTriggerEnter(entityId, otherId);
        }
        
        [UnmanagedCallersOnly(EntryPoint = "ScriptManager_OnTriggerExit", CallConventions = CallConventions.Cdecl)]
        public static void OnTriggerExit(ulong entityId, ulong otherId)
        {
            ScriptManager.OnTriggerExit(entityId, otherId);
        }
        
        [UnmanagedCallersOnly(EntryPoint = "ScriptManager_OnTransformInterpolate", CallConventions = CallConventions.Cdecl)]
        public static void OnTransformInterpolate(ulong entityId, float x, float y, float z, float alpha)
        {
            ScriptManager.OnTransformInterpolate(entityId, new Vector3(x, y, z), alpha);
        }
        
        [UnmanagedCallersOnly(EntryPoint = "ScriptManager_RegisterScript", CallConventions = CallConventions.Cdecl)]
        public static void RegisterScript([MarshalAs(UnmanagedType.LPStr)] string scriptTypeName)
        {
            var scriptType = Type.GetType(scriptTypeName);
            if (scriptType == null)
            {
                Interop.LogError($"Could not find script type: {scriptTypeName}");
                return;
            }
            
            var registerMethod = typeof(ScriptManager).GetMethod("RegisterScript");
            var genericMethod = registerMethod.MakeGenericMethod(scriptType);
            genericMethod.Invoke(null, null);
        }
    }
}
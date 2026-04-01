using System;

namespace GameEngine.SDK
{
    public class Entity
    {
        private readonly ulong _id;
        
        public Entity(ulong id)
        {
            _id = id;
        }
        
        public static Entity Invalid => new Entity(0);
        
        public bool IsValid => _id != 0;
        
        public bool HasComponent<T>() where T : new()
        {
            return Interop.HasComponent(_id, typeof(T).Name);
        }
        
        public T GetComponent<T>() where T : new()
        {
            return Interop.GetComponent<T>(_id);
        }
        
        public void AddComponent<T>() where T : new()
        {
            Interop.AddComponent<T>(_id);
        }
        
        public void RemoveComponent<T>()
        {
            Interop.RemoveComponent(_id, typeof(T).Name);
        }
        
        public void Destroy()
        {
            Interop.DestroyEntity(_id);
        }
        
        public Entity Clone()
        {
            return Interop.CloneEntity(_id);
        }
        
        public static Entity Create()
        {
            return Interop.CreateEntity();
        }
    }
    
    public static class Interop
    {
        [System.Runtime.InteropServices.DllImport("GameEngine.dll")]
        public static extern bool HasComponent(ulong entityId, [System.Runtime.InteropServices.MarshalAs(UnmanagedType.LPStr)] string typeName);
        
        [System.Runtime.InteropServices.DllImport("GameEngine.dll")]
        public static extern T GetComponent<T>(ulong entityId) where T : new();
        
        [System.Runtime.InteropServices.DllImport("GameEngine.dll")]
        public static extern void AddComponent<T>(ulong entityId) where T : new();
        
        [System.Runtime.InteropServices.DllImport("GameEngine.dll")]
        public static extern void RemoveComponent(ulong entityId, [System.Runtime.InteropServices.MarshalAs(UnmanagedType.LPStr)] string typeName);
        
        [System.Runtime.InteropServices.DllImport("GameEngine.dll")]
        public static extern void DestroyEntity(ulong entityId);
        
        [System.Runtime.InteropServices.DllImport("GameEngine.dll")]
        public static extern Entity CloneEntity(ulong entityId);
        
        [System.Runtime.InteropServices.DllImport("GameEngine.dll")]
        public static extern Entity CreateEntity();
    }
}

using System;
using System.Runtime.InteropServices;

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
        
        public static Entity Get(ulong id) => new Entity(id);
        
        public ulong GetID() => _id;
    }
    
    public static class Interop
    {
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool HasComponent(ulong entityId, [MarshalAs(UnmanagedType.LPStr)] string typeName);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr GetComponentPtr(ulong entityId, [MarshalAs(UnmanagedType.LPStr)] string typeName);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void AddComponent(ulong entityId, [MarshalAs(UnmanagedType.LPStr)] string typeName);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void RemoveComponent(ulong entityId, [MarshalAs(UnmanagedType.LPStr)] string typeName);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void DestroyEntity(ulong entityId);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern ulong CloneEntity(ulong entityId);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern ulong CreateEntity();
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void LogInfo([MarshalAs(UnmanagedType.LPStr)] string message);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void LogWarning([MarshalAs(UnmanagedType.LPStr)] string message);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void LogError([MarshalAs(UnmanagedType.LPStr)] string message);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool IsKeyPressed(int keyCode);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool IsMouseButtonPressed(int button);
    }
}
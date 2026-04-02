using System;
using System.IO;
using System.Reflection;
using System.Runtime.Loader;

namespace GameEngine.SDK
{
    public class ScriptAssemblyContext : IDisposable
    {
        private AssemblyLoadContext _loadContext;
        private Assembly _assembly;
        private string _assemblyPath;
        private bool _isDisposed;

        public Assembly Assembly => _assembly;
        public bool IsLoaded => _assembly != null && !_isDisposed;
        public string AssemblyPath => _assemblyPath;

        public ScriptAssemblyContext(string assemblyPath)
        {
            _assemblyPath = assemblyPath;
            LoadAssembly();
        }

        private void LoadAssembly()
        {
            if (_loadContext != null)
            {
                _loadContext.Unload();
                _loadContext = null;
            }

            _loadContext = new AssemblyLoadContext("ScriptContext_" + Path.GetFileNameWithoutExtension(_assemblyPath), isCollectible: true);
            _loadContext.Resolving += OnResolving;
            
            _assembly = _loadContext.LoadFromAssemblyPath(_assemblyPath);
            _isDisposed = false;

            Interop.LogInfo($"Loaded assembly: {Path.GetFileName(_assemblyPath)}");
        }

        private Assembly OnResolving(AssemblyLoadContext context, AssemblyName name)
        {
            return null;
        }

        public Type[] GetScriptTypes()
        {
            if (_assembly == null) return Array.Empty<Type>();

            var scriptTypes = new System.Collections.Generic.List<Type>();
            foreach (var type in _assembly.GetTypes())
            {
                if (type.IsSubclassOf(typeof(ScriptableEntity)) && !type.IsAbstract)
                {
                    scriptTypes.Add(type);
                }
            }
            return scriptTypes.ToArray();
        }

        public ScriptableEntity CreateScriptInstance(Type scriptType, ulong entityId)
        {
            if (_assembly == null || scriptType == null) return null;

            var script = Activator.CreateInstance(scriptType) as ScriptableEntity;
            if (script != null)
            {
                script.SetEntity(new Entity(entityId));
            }
            return script;
        }

        public void Reload()
        {
            if (!File.Exists(_assemblyPath))
            {
                Interop.LogError($"Assembly not found for reload: {_assemblyPath}");
                return;
            }

            Interop.LogInfo($"Reloading assembly: {Path.GetFileName(_assemblyPath)}");

            var oldContext = _loadContext;
            var oldAssembly = _assembly;

            try
            {
                LoadAssembly();
            }
            catch (Exception ex)
            {
                Interop.LogError($"Failed to reload assembly: {ex.Message}");
                _loadContext = oldContext;
                _assembly = oldAssembly;
                return;
            }

            oldContext?.Unload();
            _isDisposed = false;
        }

        public void Dispose()
        {
            if (_isDisposed) return;

            _loadContext?.Resolving -= OnResolving;
            _loadContext?.Unload();
            _loadContext = null;
            _assembly = null;
            _isDisposed = true;

            Interop.LogInfo($"Unloaded assembly: {Path.GetFileName(_assemblyPath)}");
        }
    }

    public class ScriptAssemblyManager
    {
        private static System.Collections.Generic.Dictionary<string, ScriptAssemblyContext> _contexts = 
            new System.Collections.Generic.Dictionary<string, ScriptAssemblyContext>();

        public static void LoadAssembly(string assemblyPath)
        {
            if (_contexts.ContainsKey(assemblyPath))
            {
                _contexts[assemblyPath].Dispose();
                _contexts.Remove(assemblyPath);
            }

            var context = new ScriptAssemblyContext(assemblyPath);
            _contexts[assemblyPath] = context;

            foreach (var scriptType in context.GetScriptTypes())
            {
                ScriptManager.RegisterScript(scriptType);
            }
        }

        public static void ReloadAssembly(string assemblyPath)
        {
            if (!_contexts.ContainsKey(assemblyPath))
            {
                LoadAssembly(assemblyPath);
                return;
            }

            var context = _contexts[assemblyPath];

            ScriptManager.OnBeforeAssemblyReload(assemblyPath);

            context.Reload();

            foreach (var scriptType in context.GetScriptTypes())
            {
                ScriptManager.RegisterScript(scriptType);
            }

            ScriptManager.OnAfterAssemblyReload(assemblyPath);
        }

        public static void UnloadAssembly(string assemblyPath)
        {
            if (_contexts.ContainsKey(assemblyPath))
            {
                _contexts[assemblyPath].Dispose();
                _contexts.Remove(assemblyPath);
            }
        }

        public static void UnloadAll()
        {
            foreach (var context in _contexts.Values)
            {
                context.Dispose();
            }
            _contexts.Clear();
        }

        public static bool IsLoaded(string assemblyPath)
        {
            return _contexts.ContainsKey(assemblyPath) && _contexts[assemblyPath].IsLoaded;
        }

        public static ScriptAssemblyContext GetContext(string assemblyPath)
        {
            return _contexts.ContainsKey(assemblyPath) ? _contexts[assemblyPath] : null;
        }
    }
}
using System;
using System.Runtime.InteropServices;

namespace GameEngine.SDK
{
    public static class Input
    {
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool Input_IsActionPressed([MarshalAs(UnmanagedType.LPStr)] string actionName);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool Input_IsActionJustPressed([MarshalAs(UnmanagedType.LPStr)] string actionName);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool Input_IsActionJustReleased([MarshalAs(UnmanagedType.LPStr)] string actionName);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern float Input_GetActionValue([MarshalAs(UnmanagedType.LPStr)] string actionName);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern float Input_GetAxis2D_X([MarshalAs(UnmanagedType.LPStr)] string axisName);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern float Input_GetAxis2D_Y([MarshalAs(UnmanagedType.LPStr)] string axisName);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern float Input_GetMouseX();
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern float Input_GetMouseY();
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern float Input_GetMouseDeltaX();
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern float Input_GetMouseDeltaY();
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern float Input_GetMouseWheel();
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool Input_IsGamepadConnected(int gamepadId);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern float Input_GetGamepadAxis(int gamepadId, int axis);
        
        [DllImport("GameEngine.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool Input_IsGamepadButtonPressed(int gamepadId, int button);
        
        public static bool IsActionPressed(string actionName) => Input_IsActionPressed(actionName);
        public static bool IsActionJustPressed(string actionName) => Input_IsActionJustPressed(actionName);
        public static bool IsActionJustReleased(string actionName) => Input_IsActionJustReleased(actionName);
        public static float GetActionValue(string actionName) => Input_GetActionValue(actionName);
        
        public static Vector3 GetAxis(string axisName)
        {
            return new Vector3(
                Input_GetAxis2D_X(axisName),
                0,
                Input_GetAxis2D_Y(axisName)
            );
        }
        
        public static Vector2 GetAxis2D(string axisName)
        {
            return new Vector2(
                Input_GetAxis2D_X(axisName),
                Input_GetAxis2D_Y(axisName)
            );
        }
        
        public static float MouseX => Input_GetMouseX();
        public static float MouseY => Input_GetMouseY();
        public static float MouseDeltaX => Input_GetMouseDeltaX();
        public static float MouseDeltaY => Input_GetMouseDeltaY();
        public static float MouseWheel => Input_GetMouseWheel();
        
        public static Vector2 MousePosition => new Vector2(MouseX, MouseY);
        public static Vector2 MouseDelta => new Vector2(MouseDeltaX, MouseDeltaY);
        
        public static bool IsGamepadConnected(int gamepadId = 0) => Input_IsGamepadConnected(gamepadId);
        
        public static float GetGamepadAxis(int axis, int gamepadId = 0) => Input_GetGamepadAxis(gamepadId, axis);
        
        public static Vector2 GetGamepadLeftStick(int gamepadId = 0)
        {
            return new Vector2(
                Input_GetGamepadAxis(gamepadId, 0),
                Input_GetGamepadAxis(gamepadId, 1)
            );
        }
        
        public static Vector2 GetGamepadRightStick(int gamepadId = 0)
        {
            return new Vector2(
                Input_GetGamepadAxis(gamepadId, 2),
                Input_GetGamepadAxis(gamepadId, 3)
            );
        }
        
        public static bool IsGamepadButtonPressed(int button, int gamepadId = 0) 
            => Input_IsGamepadButtonPressed(gamepadId, button);
        
        public static bool IsGamepadA(int gamepadId = 0) => IsGamepadButtonPressed(0, gamepadId);
        public static bool IsGamepadB(int gamepadId = 0) => IsGamepadButtonPressed(1, gamepadId);
        public static bool IsGamepadX(int gamepadId = 0) => IsGamepadButtonPressed(2, gamepadId);
        public static bool IsGamepadY(int gamepadId = 0) => IsGamepadButtonPressed(3, gamepadId);
    }
}

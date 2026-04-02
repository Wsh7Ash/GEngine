using System;
using System.Collections.Generic;
using GameEngine.SDK;

public static class MathHelper
{
    public static float Lerp(float a, float b, float t) => a + (b - a) * t;
}

public class GameDemo : ScriptableEntity
{
    public float MoveSpeed = 8.0f;
    public float SprintMultiplier = 1.8f;
    public float JumpForce = 5.0f;
    public float MouseSensitivity = 0.1f;
    public float CameraDistance = 5.0f;
    public float CameraHeight = 2.0f;
    
    private Rigidbody3DComponent _rigidbody;
    private TransformComponent _transform;
    private float _yaw = 0.0f;
    private float _pitch = 0.0f;
    private bool _isGrounded = false;
    private bool _isPaused = false;
    private int _score = 0;
    private float _gameTime = 0.0f;
    private Entity _cameraEntity;
    private TransformComponent _cameraTransform;
    private List<Entity> _collectibles = new List<Entity>();
    private Random _random = new Random();
    
    private float _moveX = 0f;
    private float _moveZ = 0f;
    private bool _jumpPressed = false;
    private bool _sprintPressed = false;
    
    public override void OnCreate()
    {
        Log("=== GAME DEMO STARTED ===");
        Log("Controls: WASD - Move, Space - Jump, Mouse - Look, Shift - Sprint, ESC - Pause");
        
        _transform = GetComponent<TransformComponent>();
        
        if (!HasComponent<Rigidbody3DComponent>())
        {
            AddComponent<Rigidbody3DComponent>();
        }
        
        _rigidbody = GetComponent<Rigidbody3DComponent>();
        _rigidbody.MotionType = Rigidbody3DMotionType.Dynamic;
        _rigidbody.Mass = 70.0f;
        _rigidbody.LinearDamping = 0.1f;
        _rigidbody.AngularDamping = 0.99f;
        _rigidbody.Friction = 0.5f;
        
        CreateCamera();
        SpawnCollectibles(10);
        
        Log($"Player initialized at position: {_transform.Position}");
    }
    
    private void CreateCamera()
    {
        _cameraEntity = CreateEntity();
        _cameraEntity.AddComponent<TransformComponent>();
        _cameraTransform = _cameraEntity.GetComponent<TransformComponent>();
        _cameraTransform.Position = new Vector3(0, CameraHeight, CameraDistance);
        Log("Camera created and attached");
    }
    
    private void SpawnCollectibles(int count)
    {
        for (int i = 0; i < count; i++)
        {
            Entity collectible = CreateEntity();
            collectible.AddComponent<TransformComponent>();
            
            var transform = collectible.GetComponent<TransformComponent>();
            float x = (float)(_random.NextDouble() * 40 - 20);
            float z = (float)(_random.NextDouble() * 40 - 20);
            transform.Position = new Vector3(x, 1.0f, z);
            
            _collectibles.Add(collectible);
        }
        Log($"Spawned {count} collectibles");
    }
    
    public override void OnUpdate(float deltaTime)
    {
        if (_isPaused) return;
        
        _gameTime += deltaTime;
        
        ProcessInput();
        
        MovePlayer(deltaTime);
        
        UpdateCamera();
        
        CheckCollectibles();
        
        HandlePause();
        
        UpdateHUD();
    }
    
    private void ProcessInput()
    {
        _moveX = 0f;
        _moveZ = 0f;
        
        if (IsKeyPressed(65)) _moveX -= 1;
        if (IsKeyPressed(68)) _moveX += 1;
        if (IsKeyPressed(87)) _moveZ -= 1;
        if (IsKeyPressed(83)) _moveZ += 1;
        
        _jumpPressed = IsKeyPressed(32);
        _sprintPressed = IsKeyPressed(340);
        
        _yaw += Input.MouseDeltaX * MouseSensitivity;
        _pitch -= Input.MouseDeltaY * MouseSensitivity;
        _pitch = Math.Max(-89f, Math.Min(89f, _pitch));
    }
    
    private void MovePlayer(float deltaTime)
    {
        float speed = MoveSpeed;
        if (_sprintPressed) speed *= SprintMultiplier;
        
        Vector3 forward = new Vector3(
            (float)Math.Sin(_yaw * Math.PI / 180.0f),
            0,
            (float)Math.Cos(_yaw * Math.PI / 180.0f)
        );
        
        Vector3 right = new Vector3(
            (float)Math.Cos(_yaw * Math.PI / 180.0f),
            0,
            -(float)Math.Sin(_yaw * Math.PI / 180.0f)
        );
        
        Vector3 moveDirection = (forward * -_moveZ + right * _moveX).Normalized;
        
        Vector3 currentVelocity = _rigidbody.Velocity;
        
        float targetSpeedX = moveDirection.X * speed;
        float targetSpeedZ = moveDirection.Z * speed;
        
        float newVelX = MathHelper.Lerp(currentVelocity.X, targetSpeedX, 0.1f);
        float newVelZ = MathHelper.Lerp(currentVelocity.Z, targetSpeedZ, 0.1f);
        
        _rigidbody.Velocity = new Vector3(newVelX, currentVelocity.Y, newVelZ);
        
        if (_jumpPressed && Math.Abs(currentVelocity.Y) < 0.1f)
        {
            _rigidbody.AddImpulse(0, JumpForce, 0);
        }
    }
    
    private void UpdateCamera()
    {
        if (_cameraTransform == null) return;
        
        float yawRad = _yaw * Math.PI / 180.0f;
        float pitchRad = _pitch * Math.PI / 180.0f;
        
        float offsetX = (float)(CameraDistance * Math.Sin(yawRad) * Math.Cos(pitchRad));
        float offsetY = (float)(CameraDistance * Math.Sin(pitchRad) + CameraHeight);
        float offsetZ = (float)(CameraDistance * Math.Cos(yawRad) * Math.Cos(pitchRad));
        
        Vector3 targetPos = _transform.Position;
        
        _cameraTransform.Position = new Vector3(
            targetPos.X - offsetX,
            targetPos.Y - offsetY,
            targetPos.Z - offsetZ
        );
        
        Vector3 lookAt = targetPos;
        Vector3 camPos = _cameraTransform.Position;
        Vector3 forward = (lookAt - camPos).Normalized;
        
        float yaw = (float)(Math.Atan2(forward.X, forward.Z) * 180.0f / Math.PI);
        float pitch = (float)(Math.Asin(forward.Y) * 180.0f / Math.PI);
        
        _cameraTransform.EulerAngles = new Vector3(pitch, yaw, 0);
    }
    
    private void CheckCollectibles()
    {
        Vector3 playerPos = _transform.Position;
        
        for (int i = _collectibles.Count - 1; i >= 0; i--)
        {
            Entity collectible = _collectibles[i];
            if (!collectible.IsValid)
            {
                _collectibles.RemoveAt(i);
                continue;
            }
            
            var transform = collectible.GetComponent<TransformComponent>();
            float dist = (transform.Position - playerPos).Magnitude;
            
            if (dist < 1.5f)
            {
                collectible.Destroy();
                _collectibles.RemoveAt(i);
                _score += 10;
                Log($"Collected! Score: {_score}");
            }
        }
        
        if (_collectibles.Count == 0)
        {
            Log("All collectibles gathered! Spawning more...");
            SpawnCollectibles(10);
        }
    }
    
    private void HandlePause()
    {
        if (IsActionJustPressed("Pause"))
        {
            _isPaused = !_isPaused;
            Log(_isPaused ? "Game Paused" : "Game Resumed");
        }
    }
    
    private void UpdateHUD()
    {
    }
    
    public override void OnCollisionEnter(Entity other)
    {
        Log($"Collision with entity {other.GetID()}");
    }
    
    public override void OnDestroy()
    {
        Log($"Game Over! Final Score: {_score}, Time: {_gameTime:F1}s");
        Log("=== GAME DEMO ENDED ===");
        
        if (_cameraEntity.IsValid)
        {
            _cameraEntity.Destroy();
        }
        
        foreach (var c in _collectibles)
        {
            if (c.IsValid) c.Destroy();
        }
    }
}

public class Collectible : ScriptableEntity
{
    public float RotateSpeed = 45.0f;
    public float BobSpeed = 2.0f;
    public float BobAmount = 0.2f;
    
    private TransformComponent _transform;
    private float _startY;
    private float _time = 0f;
    
    public override void OnCreate()
    {
        _transform = GetComponent<TransformComponent>();
        _startY = _transform.Position.Y;
        
        AddComponent<Rigidbody3DComponent>();
        var rb = GetComponent<Rigidbody3DComponent>();
        rb.MotionType = Rigidbody3DMotionType.Static;
        rb.IsSensor = true;
        
        Log($"Collectible created at {_transform.Position}");
    }
    
    public override void OnUpdate(float deltaTime)
    {
        _time += deltaTime;
        
        _transform.EulerAngles = new Vector3(
            0,
            _transform.EulerAngles.Y + RotateSpeed * deltaTime,
            0
        );
        
        float newY = _startY + (float)Math.Sin(_time * BobSpeed) * BobAmount;
        _transform.Position = new Vector3(
            _transform.Position.X,
            newY,
            _transform.Position.Z
        );
    }
}

public class RotatorDemo : ScriptableEntity
{
    public float Speed = 50.0f;
    public Vector3 Axis = Vector3.Up;
    
    public override void OnUpdate(float deltaTime)
    {
        var transform = GetComponent<TransformComponent>();
        Quaternion currentRotation = transform.Rotation;
        Quaternion deltaRotation = Quaternion.FromEuler(
            Axis.X * Speed * deltaTime,
            Axis.Y * Speed * deltaTime,
            Axis.Z * Speed * deltaTime
        );
        transform.Rotation = deltaRotation * currentRotation;
    }
}

public class MoverDemo : ScriptableEntity
{
    public float Speed = 2.0f;
    public float Range = 3.0f;
    
    private Vector3 _startPos;
    private float _time = 0f;
    
    public override void OnCreate()
    {
        _startPos = Position;
    }
    
    public override void OnUpdate(float deltaTime)
    {
        _time += deltaTime;
        float offset = (float)Math.Sin(_time * Speed) * Range;
        Position = new Vector3(_startPos.X + offset, _startPos.Y, _startPos.Z);
    }
}

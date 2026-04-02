using GameEngine.SDK;

public class Rotator : ScriptableEntity
{
    private float _speed = 50.0f;
    private Vector3 _axis = Vector3.Up;
    
    public float Speed
    {
        get => _speed;
        set => _speed = value;
    }
    
    public Vector3 Axis
    {
        get => _axis;
        set => _axis = value;
    }
    
    public override void OnCreate()
    {
        Log($"Rotator script created on entity {Entity.GetID()}");
    }
    
    public override void OnUpdate(float deltaTime)
    {
        var transform = GetComponent<TransformComponent>();
        
        Quaternion currentRotation = transform.Rotation;
        Quaternion deltaRotation = Quaternion.FromEuler(
            _axis.X * _speed * deltaTime,
            _axis.Y * _speed * deltaTime,
            _axis.Z * _speed * deltaTime
        );
        
        transform.Rotation = deltaRotation * currentRotation;
    }
    
    public override void OnDestroy()
    {
        Log($"Rotator destroyed on entity {Entity.GetID()}");
    }
    
    public override void OnCollisionEnter(Entity other)
    {
        LogWarning($"Rotator collided with entity {other.GetID()}");
    }
}

public class PlayerController : ScriptableEntity
{
    private float _moveSpeed = 5.0f;
    private float _sprintMultiplier = 2.0f;
    
    public float MoveSpeed
    {
        get => _moveSpeed;
        set => _moveSpeed = value;
    }
    
    public override void OnCreate()
    {
        Log("Player controller initialized");
        
        if (!HasComponent<Rigidbody3DComponent>())
        {
            AddComponent<Rigidbody3DComponent>();
        }
        
        var rb = GetComponent<Rigidbody3DComponent>();
        rb.MotionType = Rigidbody3DMotionType.Dynamic;
        rb.Mass = 70.0f;
        rb.LinearDamping = 0.1f;
    }
    
    public override void OnUpdate(float deltaTime)
    {
        float moveX = 0f;
        float moveZ = 0f;
        
        if (IsKeyPressed(65)) moveX -= 1;  // A
        if (IsKeyPressed(68)) moveX += 1;  // D
        if (IsKeyPressed(87)) moveZ -= 1;  // W
        if (IsKeyPressed(83)) moveZ += 1;  // S
        
        float speed = _moveSpeed;
        if (IsKeyPressed(340))  // Left Shift
        {
            speed *= _sprintMultiplier;
        }
        
        var transform = GetComponent<TransformComponent>();
        
        transform.Position = new Vector3(
            transform.Position.X + moveX * speed * deltaTime,
            transform.Position.Y,
            transform.Position.Z + moveZ * speed * deltaTime
        );
    }
    
    public override void OnCollisionEnter(Entity other)
    {
        Log($"Player collided with {other.GetID()}");
    }
}

public class EntitySpawner : ScriptableEntity
{
    private float _spawnTimer = 0f;
    private float _spawnInterval = 2.0f;
    private int _spawnedCount = 0;
    private const int MaxSpawns = 10;
    private string _prefabTag = "Cube";
    
    public float SpawnInterval
    {
        get => _spawnInterval;
        set => _spawnInterval = value;
    }
    
    public string PrefabTag
    {
        get => _prefabTag;
        set => _prefabTag = value;
    }
    
    public override void OnCreate()
    {
        Log("Entity spawner started");
    }
    
    public override void OnUpdate(float deltaTime)
    {
        if (_spawnedCount >= MaxSpawns) 
        {
            Log($"Max spawns reached ({MaxSpawns})");
            return;
        }
        
        _spawnTimer += deltaTime;
        if (_spawnTimer >= _spawnInterval)
        {
            _spawnTimer = 0f;
            
            var newEntity = CreateEntity();
            
            newEntity.AddComponent<TransformComponent>();
            var transform = newEntity.GetComponent<TransformComponent>();
            transform.Position = new Vector3(
                Position.X + (_spawnedCount % 5) * 2.0f,
                Position.Y,
                Position.Z + (_spawnedCount / 5) * 2.0f
            );
            
            _spawnedCount++;
            Log($"Spawned entity {newEntity.GetID()} (total: {_spawnedCount})");
        }
    }
}

public class RotatorWithPhysics : ScriptableEntity
{
    public float Speed { get; set; } = 30.0f;
    public Vector3 Axis { get; set; } = Vector3.Up;
    
    public override void OnCreate()
    {
        if (!HasComponent<Rigidbody3DComponent>())
        {
            AddComponent<Rigidbody3DComponent>();
        }
        
        var rb = GetComponent<Rigidbody3DComponent>();
        rb.MotionType = Rigidbody3DMotionType.Kinematic;
        rb.Mass = 1.0f;
        
        Log("RotatorWithPhysics created with kinematic rigidbody");
    }
    
    public override void OnUpdate(float dt)
    {
        var transform = GetComponent<TransformComponent>();
        
        Quaternion currentRotation = transform.Rotation;
        Quaternion deltaRotation = Quaternion.FromEuler(
            Axis.X * Speed * dt,
            Axis.Y * Speed * dt,
            Axis.Z * Speed * dt
        );
        
        transform.Rotation = deltaRotation * currentRotation;
    }
}
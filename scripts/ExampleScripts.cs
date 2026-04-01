using GameEngine.SDK;

public class Rotator : ScriptableEntity
{
    private float _speed = 50.0f;
    
    public float Speed
    {
        get => _speed;
        set => _speed = value;
    }
    
    public override void OnCreate()
    {
        Log($"Rotator created on entity {entity.GetID()}");
    }
    
    public override void OnUpdate(float deltaTime)
    {
        var transform = GetComponent<TransformComponent>();
        
        float x = transform.Rotation.X + _speed * deltaTime;
        transform.Rotation = new Vector3(x, transform.Rotation.Y, transform.Rotation.Z);
    }
}

public class PlayerController : ScriptableEntity
{
    private float _moveSpeed = 5.0f;
    private bool _isGrounded = false;
    
    public override void OnCreate()
    {
        Log("Player controller initialized");
    }
    
    public override void OnUpdate(float deltaTime)
    {
        float moveX = 0f;
        float moveZ = 0f;
        
        if (IsKeyPressed(65)) moveX -= 1;  // A key
        if (IsKeyPressed(68)) moveX += 1;  // D key
        if (IsKeyPressed(87)) moveZ -= 1;  // W key
        if (IsKeyPressed(83)) moveZ += 1;  // S key
        
        var transform = GetComponent<TransformComponent>();
        
        transform.Position = new Vector3(
            transform.Position.X + moveX * _moveSpeed * deltaTime,
            transform.Position.Y,
            transform.Position.Z + moveZ * _moveSpeed * deltaTime
        );
    }
}

public class EntitySpawner : ScriptableEntity
{
    private float _spawnTimer = 0f;
    private float _spawnInterval = 2.0f;
    private int _spawnedCount = 0;
    private const int MaxSpawns = 10;
    
    public override void OnCreate()
    {
        Log("Entity spawner started");
    }
    
    public override void OnUpdate(float deltaTime)
    {
        if (_spawnedCount >= MaxSpawns) return;
        
        _spawnTimer += deltaTime;
        if (_spawnTimer >= _spawnInterval)
        {
            _spawnTimer = 0f;
            
            var newEntity = CreateEntity();
            Log($"Spawned new entity: {newEntity.GetID()}");
            
            _spawnedCount++;
        }
    }
}
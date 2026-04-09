#include "PlayerSetup.h"
#include "CameraController.h"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>

namespace ge {
namespace demo {

struct PhysicsDemoConfig {
    std::string scene = "basic";
    float moveSpeed = 5.0f;
    float runMultiplier = 2.0f;
    bool showStats = true;
};

class PhysicsDemo {
public:
    PhysicsDemo(const PhysicsDemoConfig& config);
    ~PhysicsDemo();

    void Run();
    void Shutdown();

private:
    void Initialize();
    void Update(float dt);
    void ProcessInput(float dt);
    void UpdateCamera(float dt);
    void PrintStats();

    PhysicsDemoConfig config_;
    ecs::World world_;
    ecs::Entity player_;
    FollowCamera camera_;

    bool running_ = false;
    bool initialized_ = false;
    float accumulator_ = 0.0f;
    float tickInterval_ = 1.0f / 60.0f;

    int frameCount_ = 0;
    float statsTimer_ = 0.0f;
    float fps_ = 0.0f;
    float avgFrameTime_ = 0.0f;

    struct KeyState {
        bool forward = false;
        bool backward = false;
        bool left = false;
        bool right = false;
        bool jump = false;
        bool sprint = false;
        bool crouch = false;
    };

    KeyState keys_;
    float mouseDX_ = 0.0f;
    float mouseDY_ = 0.0f;
    float playerYaw_ = 0.0f;
};

PhysicsDemo::PhysicsDemo(const PhysicsDemoConfig& config)
    : config_(config), camera_() {
}

PhysicsDemo::~PhysicsDemo() {
    Shutdown();
}

void PhysicsDemo::Run() {
    if (running_) return;

    Initialize();
    running_ = true;

    std::cout << "GEngine Physics Demo - " << config_.scene << " scene\n";
    std::cout << "Controls: WASD=Move, Space=Jump, Shift=Sprint, Ctrl=Crouch, Mouse=Look, ESC=Quit\n\n";

    auto lastTime = std::chrono::high_resolution_clock::now();

    while (running_) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        if (dt > 0.1f) dt = 0.1f;

        accumulator_ += dt;
        while (accumulator_ >= tickInterval_) {
            Update(tickInterval_);
            accumulator_ -= tickInterval_;
        }

        UpdateCamera(dt);

        if (config_.showStats) {
            statsTimer_ += dt;
            frameCount_++;
            if (statsTimer_ >= 1.0f) {
                PrintStats();
                statsTimer_ = 0.0f;
                frameCount_ = 0;
            }
        }
    }
}

void PhysicsDemo::Shutdown() {
    if (!running_) return;
    running_ = false;
    std::cout << "PhysicsDemo shut down.\n";
}

void PhysicsDemo::Initialize() {
    if (initialized_) return;

    std::cout << "Initializing physics demo...\n";

    player_ = CreatePlayerEntity(world_, PhysicsDemoPlayerConfig{
        1.8f, 0.4f, 70.0f,
        config_.moveSpeed,
        config_.runMultiplier,
        0.5f,
        5.0f,
        45.0f
    });

    CreateStaticBox(world_, Math::Vec3f(0.0f, -0.5f, 0.0f), Math::Vec3f(50.0f, 1.0f, 50.0f));

    if (config_.scene == "basic") {
    }
    else if (config_.scene == "playground") {
        CreateStaticBox(world_, Math::Vec3f(5.0f, 0.5f, 0.0f), Math::Vec3f(2.0f, 1.0f, 2.0f));
        CreateStaticBox(world_, Math::Vec3f(-5.0f, 0.5f, 0.0f), Math::Vec3f(2.0f, 1.0f, 2.0f));
        CreateRamp(world_, Math::Vec3f(0.0f, 0.0f, 8.0f), 4.0f, 6.0f, 20.0f);
        for (int i = 0; i < 5; ++i) {
            CreateDynamicBox(world_, Math::Vec3f(-3.0f + i * 1.5f, 2.0f + i * 0.5f, -5.0f), 
                            Math::Vec3f(1.0f, 1.0f, 1.0f), 1.0f);
        }
    }
    else if (config_.scene == "benchmark") {
        for (int x = -5; x <= 5; ++x) {
            for (int z = -5; z <= 5; ++z) {
                CreateDynamicBox(world_, Math::Vec3f(x * 2.0f, 5.0f + z * 0.5f, -10.0f),
                               Math::Vec3f(1.0f, 1.0f, 1.0f), 1.0f);
            }
        }
    }

    auto& transform = world_.GetComponent<TransformComponent>(player_);
    camera_.SetTarget(transform.position.x, transform.position.y, transform.position.z);

    initialized_ = true;
}

void PhysicsDemo::Update(float dt) {
    ProcessInput(dt);

    world_.GetSystem<ecs::Physics3DSystem>()->Update(*world_, dt);
}

void PhysicsDemo::ProcessInput(float dt) {
    if (!world_.HasComponent<InputStateComponent>(player_)) return;

    auto& input = world_.GetComponent<InputStateComponent>(player_);

    float moveX = 0.0f;
    float moveZ = 0.0f;

    if (keys_.forward) moveZ -= 1.0f;
    if (keys_.backward) moveZ += 1.0f;
    if (keys_.left) moveX -= 1.0f;
    if (keys_.right) moveX += 1.0f;

    float len = moveX * moveX + moveZ * moveZ;
    if (len > 0.0f) {
        len = std::sqrt(len);
        moveX /= len;
        moveZ /= len;
    }

    float cosYaw = std::cos(playerYaw_);
    float sinYaw = std::sin(playerYaw_);
    float rotatedX = moveX * cosYaw - moveZ * sinYaw;
    float rotatedZ = moveX * sinYaw + moveZ * cosYaw;

    input.Move = Math::Vec3f(rotatedX, 0.0f, rotatedZ);
    input.Jump = keys_.jump;
    input.Sprint = keys_.sprint;
    input.Crouch = keys_.crouch;

    playerYaw_ += mouseDX_ * 0.2f;
    mouseDX_ = 0.0f;
    mouseDY_ = 0.0f;
}

void PhysicsDemo::UpdateCamera(float dt) {
    if (!world_.HasComponent<TransformComponent>(player_)) return;

    auto& transform = world_.GetComponent<TransformComponent>(player_);
    camera_.SetTarget(transform.position.x, transform.position.y + 1.0f, transform.position.z);
    camera_.SetRotation(playerYaw_, 20.0f);
    camera_.Update(dt);
}

void PhysicsDemo::PrintStats() {
    std::cout << "FPS: " << frameCount_ 
              << " | Camera: (" << camera_.GetX() << ", " << camera_.GetY() << ", " << camera_.GetZ() << ")"
              << " | Target: (" << camera_.GetTargetX() << ", " << camera_.GetTargetY() << ", " << camera_.GetTargetZ() << ")"
              << "\n";
}

} // namespace demo
} // namespace ge
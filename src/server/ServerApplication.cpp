#include "ServerApplication.h"
#include "../core/debug/log.h"
#include "../core/ecs/systems/Physics2DSystem.h"
#include "../core/ecs/systems/Physics3DSystem.h"
#include "../core/ecs/systems/ScriptSystem.h"
#include "../core/ecs/systems/BehaviorTreeSystem.h"
#include "../core/ecs/systems/VisualScriptSystem.h"
#include "../core/ecs/systems/BuoyancySystem.h"
#include "../core/ecs/systems/VehicleSystem.h"
#include "../core/ecs/systems/DestructibleSystem.h"
#include "../core/ecs/systems/ClothSystem.h"
#include "../core/ecs/systems/AnimationSystem.h"
#include "../core/ecs/systems/ParticleSystem.h"

#include "../core/ecs/components/Rigidbody2DComponent.h"
#include "../core/ecs/components/Rigidbody3DComponent.h"
#include "../core/ecs/components/NativeScriptComponent.h"
#include "../core/behaviortree/BehaviorTreeComponent.h"
#include "../core/visualscripting/VisualScriptComponent.h"
#include "../core/ecs/components/BuoyancyComponent.h"
#include "../core/ecs/components/VehicleComponent.h"
#include "../core/ecs/components/JointComponent.h"
#include "../core/ecs/components/ClothComponent.h"
#include "../core/ecs/components/AnimatorComponent.h"
#include "../core/ecs/components/ParticleEmitterComponent.h"
#include "../core/ecs/components/TransformComponent.h"

#include <chrono>
#include <thread>
#include <bitset>

namespace ge {

ServerApplication::ServerApplication() 
    : world_(std::make_unique<ecs::World>()) {
    net::SetGameState(net::GameState::Server);
    networkManager_ = std::make_unique<net::NetworkManager>();
    replicationManager_ = std::make_unique<net::ReplicationManager>();

    std::bitset<128> signature;

    auto physics2DSystem = world_->RegisterSystem<ecs::Physics2DSystem>();
    {
        signature.reset();
        signature.set(ecs::GetComponentTypeID<ecs::Rigidbody2DComponent>());
        signature.set(ecs::GetComponentTypeID<ecs::TransformComponent>());
        world_->SetSystemSignature<ecs::Physics2DSystem>(signature);
    }

    auto physics3DSystem = world_->RegisterSystem<ecs::Physics3DSystem>();
    {
        signature.reset();
        signature.set(ecs::GetComponentTypeID<ecs::Rigidbody3DComponent>());
        signature.set(ecs::GetComponentTypeID<ecs::TransformComponent>());
        world_->SetSystemSignature<ecs::Physics3DSystem>(signature);
    }

    auto scriptSystem = world_->RegisterSystem<ecs::ScriptSystem>();
    {
        signature.reset();
        signature.set(ecs::GetComponentTypeID<ecs::NativeScriptComponent>());
        world_->SetSystemSignature<ecs::ScriptSystem>(signature);
    }

    auto behaviorTreeSystem = world_->RegisterSystem<ecs::BehaviorTreeSystem>();
    {
        signature.reset();
        signature.set(ecs::GetComponentTypeID<ecs::BehaviorTreeComponent>());
        world_->SetSystemSignature<ecs::BehaviorTreeSystem>(signature);
    }

    auto visualScriptSystem = world_->RegisterSystem<ecs::VisualScriptSystem>();
    {
        signature.reset();
        signature.set(ecs::GetComponentTypeID<ecs::VisualScriptComponent>());
        world_->SetSystemSignature<ecs::VisualScriptSystem>(signature);
    }

    auto buoyancySystem = world_->RegisterSystem<ecs::BuoyancySystem>();
    {
        signature.reset();
        signature.set(ecs::GetComponentTypeID<ecs::BuoyancyComponent>());
        signature.set(ecs::GetComponentTypeID<ecs::WaterVolumeComponent>());
        world_->SetSystemSignature<ecs::BuoyancySystem>(signature);
    }

    auto vehicleSystem = world_->RegisterSystem<ecs::VehicleSystem>();
    {
        signature.reset();
        signature.set(ecs::GetComponentTypeID<ecs::VehicleComponent>());
        world_->SetSystemSignature<ecs::VehicleSystem>(signature);
    }

    auto destructibleSystem = world_->RegisterSystem<ecs::DestructibleSystem>();
    {
        signature.reset();
        signature.set(ecs::GetComponentTypeID<ecs::DestructibleComponent>());
        world_->SetSystemSignature<ecs::DestructibleSystem>(signature);
    }

    auto clothSystem = world_->RegisterSystem<ecs::ClothSystem>();
    {
        signature.reset();
        signature.set(ecs::GetComponentTypeID<ecs::ClothComponent>());
        world_->SetSystemSignature<ecs::ClothSystem>(signature);
    }

    auto animationSystem = world_->RegisterSystem<ecs::AnimationSystem>();
    {
        signature.reset();
        signature.set(ecs::GetComponentTypeID<ecs::AnimatorComponent>());
        world_->SetSystemSignature<ecs::AnimationSystem>(signature);
    }

    auto particleSystem = world_->RegisterSystem<ecs::ParticleSystem>();
    {
        signature.reset();
        signature.set(ecs::GetComponentTypeID<ecs::ParticleEmitterComponent>());
        world_->SetSystemSignature<ecs::ParticleSystem>(signature);
    }

    debug::log::info("[Server] Registered {} systems", 11);
}

ServerApplication::~ServerApplication() {
    Shutdown();
}

void ServerApplication::Run() {
    if (running_) return;
    
    running_ = true;
    InitializeServer();
    OnInitialize();

    debug::log::info("Server started on port {}", serverPort_);

    auto lastTime = std::chrono::high_resolution_clock::now();

    while (running_) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        accumulator_ += dt;
        
        while (accumulator_ >= tickInterval_) {
            Update(tickInterval_);
            accumulator_ -= tickInterval_;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    OnShutdown();
    debug::log::info("Server shut down");
}

void ServerApplication::Shutdown() {
    if (!running_) return;
    
    running_ = false;
    
    if (networkManager_) {
        networkManager_->CloseServer();
        networkManager_->Shutdown();
    }
}

void ServerApplication::InitializeServer() {
    if (serverInitialized_) return;

    networkManager_->Initialize(maxPlayers_);
    networkManager_->SetTickRate(tickRate_);
    
    auto server = networkManager_->CreateServer(serverPort_);
    if (!server) {
        debug::log::error("Failed to create server on port {}", serverPort_);
        return;
    }

    networkManager_->onClientConnected = [this](net::Connection* conn) {
        HandleClientConnect(conn);
    };
    
    networkManager_->onClientDisconnected = [this](net::Connection* conn) {
        HandleClientDisconnect(conn);
    };
    
    networkManager_->onMessageReceived = [this](net::Connection* conn, const net::Message& msg) {
        HandleNetworkMessage(conn, msg);
    };

    replicationManager_->Initialize(networkManager_.get());
    
    serverInitialized_ = true;
}

void ServerApplication::Update(float dt) {
    world_->GetSystem<ecs::Physics2DSystem>()->Update(*world_, dt);
    world_->GetSystem<ecs::Physics3DSystem>()->Update(*world_, dt);
    world_->GetSystem<ecs::ScriptSystem>()->Update(*world_, dt);
    world_->GetSystem<ecs::BehaviorTreeSystem>()->Update(*world_, dt);
    world_->GetSystem<ecs::VisualScriptSystem>()->Update(*world_, dt);
    world_->GetSystem<ecs::BuoyancySystem>()->Update(*world_, dt);
    world_->GetSystem<ecs::VehicleSystem>()->Update(*world_, dt);
    world_->GetSystem<ecs::DestructibleSystem>()->Update(*world_, dt);
    world_->GetSystem<ecs::ClothSystem>()->Update(*world_, dt);
    world_->GetSystem<ecs::AnimationSystem>()->Update(*world_, dt);
    world_->GetSystem<ecs::ParticleSystem>()->Update(*world_, dt);

    networkManager_->Update(dt);
    replicationManager_->Update(dt);
    OnUpdate(dt);
}

void ServerApplication::HandleClientConnect(net::Connection* conn) {
    debug::log::info("Client connected: {}", conn->GetId());
    OnClientConnect(conn);
    if (onClientConnected) {
        onClientConnected(conn);
    }
}

void ServerApplication::HandleClientDisconnect(net::Connection* conn) {
    debug::log::info("Client disconnected: {}", conn->GetId());
    OnClientDisconnect(conn);
    if (onClientDisconnected) {
        onClientDisconnected(conn);
    }
}

void ServerApplication::HandleNetworkMessage(net::Connection* conn, const net::Message& msg) {
    if (onMessageReceived) {
        onMessageReceived(conn, msg);
    }
}

void ServerApplication::SetMaxPlayers(int32_t max) {
    maxPlayers_ = max;
}

int32_t ServerApplication::GetMaxPlayers() const {
    return maxPlayers_;
}

void ServerApplication::SetTickRate(int32_t ticksPerSecond) {
    tickRate_ = ticksPerSecond;
    tickInterval_ = 1.0f / static_cast<float>(ticksPerSecond);
    if (networkManager_) {
        networkManager_->SetTickRate(ticksPerSecond);
    }
}

int32_t ServerApplication::GetTickRate() const {
    return tickRate_;
}

void ServerApplication::SetPort(uint16_t port) {
    serverPort_ = port;
}

uint16_t ServerApplication::GetPort() const {
    return serverPort_;
}

void ServerApplication::LoadWorld(const std::string& scenePath) {
    loadedWorldPath_ = scenePath;
    debug::log::info("Loading world from: {}", scenePath);
}

void ServerApplication::LoadWorldFromData(const std::vector<uint8_t>& sceneData) {
    worldData_ = sceneData;
    debug::log::info("Loading world from data ({} bytes)", sceneData.size());
}

} // namespace ge

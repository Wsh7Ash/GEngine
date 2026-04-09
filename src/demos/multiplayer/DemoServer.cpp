#include "DemoServer.h"
#include "../core/debug/log.h"
#include "../core/ecs/components/TransformComponent.h"
#include <chrono>
#include <thread>
#include <cstring>

namespace ge {
namespace demo {

DemoServer::DemoServer() 
    : tickInterval_(1.0f / static_cast<float>(tickRate_)) {
}

DemoServer::~DemoServer() {
    Shutdown();
}

void DemoServer::Run() {
    if (running_) return;
    
    Initialize();
    running_ = true;
    
    GE_LOG_INFO("DemoServer started on port {}", port_);
    
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
    
    GE_LOG_INFO("DemoServer shut down");
}

void DemoServer::Shutdown() {
    if (!running_) return;
    
    running_ = false;
    
    if (networkManager_) {
        networkManager_->CloseServer();
        networkManager_->Shutdown();
    }
}

void DemoServer::Initialize() {
    if (initialized_) return;
    
    net::SetGameState(net::GameState::Server);
    
    networkManager_ = std::make_unique<net::NetworkManager>();
    networkManager_->Initialize(8);
    networkManager_->SetTickRate(tickRate_);
    
    auto server = networkManager_->CreateServer(port_);
    if (!server) {
        GE_LOG_ERROR("Failed to create server on port {}", port_);
        return;
    }
    
    networkManager_->onClientConnected = [this](net::Connection* conn) {
        uint32_t clientId = conn->GetId();
        GE_LOG_INFO("Client connected: {}", clientId);
        
        uint32_t playerId = SpawnPlayer(clientId);
        
        if (onClientConnect) {
            onClientConnect(clientId);
        }
        if (onPlayerSpawn) {
            onPlayerSpawn(clientId, playerId);
        }
    };
    
    networkManager_->onClientDisconnected = [this](net::Connection* conn) {
        uint32_t clientId = conn->GetId();
        GE_LOG_INFO("Client disconnected: {}", clientId);
        
        RemovePlayer(clientId);
        
        if (onClientDisconnect) {
            onClientDisconnect(clientId);
        }
    };
    
    networkManager_->onMessageReceived = [this](net::Connection* conn, const net::Message& msg) {
        OnNetworkMessage(conn->GetId(), msg.GetData(), msg.GetSize());
    };
    
    replicationManager_ = std::make_unique<net::ReplicationManager>();
    replicationManager_->Initialize(networkManager_.get());
    
    interestManager_ = std::make_unique<net::InterestManager>();
    interestManager_->SetSpatialIndex(nullptr);
    
    initialized_ = true;
}

void DemoServer::Update(float dt) {
    if (!networkManager_) return;
    
    networkManager_->Update(dt);
    replicationManager_->Update(dt);
    
    BroadcastPlayerState();
}

void DemoServer::OnNetworkMessage(uint32_t clientId, const void* data, size_t size) {
    if (size < sizeof(InputComponent)) return;
    
    const InputComponent* input = static_cast<const InputComponent*>(data);
    ProcessClientInput(clientId, *input);
}

uint32_t DemoServer::SpawnPlayer(uint32_t clientId) {
    uint32_t playerId = nextPlayerId_++;
    
    ecs::Entity entity = world_.CreateEntity();
    
    world_.AddComponent(entity, demo::PlayerComponent(playerId, "Player" + std::to_string(playerId)));
    world_.AddComponent(entity, demo::InputComponent());
    
    world_.AddComponent(entity, ecs::TransformComponent{
        Math::Vec3f(static_cast<float>(clientId * 5), 0.0f, 0.0f),
        Math::Quatf::Identity(),
        Math::Vec3f(1.0f, 1.0f, 1.0f)
    });
    
    ecs::NetworkEntity netEntity;
    netEntity.networkId = nextEntityId_++;
    netEntity.authority = ecs::NetworkAuthority::Server;
    netEntity.replicated = true;
    netEntity.predictive = false;
    world_.AddComponent(entity, netEntity);
    
    clientEntities_[clientId] = entity;
    entityClients_[entity] = clientId;
    
    GE_LOG_INFO("Spawned player {} for client {}", playerId, clientId);
    
    return playerId;
}

void DemoServer::RemovePlayer(uint32_t clientId) {
    auto it = clientEntities_.find(clientId);
    if (it != clientEntities_.end()) {
        world_.DestroyEntity(it->second);
        entityClients_.erase(it->second);
        clientEntities_.erase(it);
        
        GE_LOG_INFO("Removed player for client {}", clientId);
    }
}

void DemoServer::BroadcastPlayerState() {
    for (const auto& [clientId, entity] : clientEntities_) {
        if (!world_.HasComponent<ecs::TransformComponent>(entity)) continue;
        
        auto& transform = world_.GetComponent<ecs::TransformComponent>(entity);
        
        net::Message msg;
        msg.SetType(net::MessageType::EntityUpdate);
        
        char buffer[256];
        uint32_t* idPtr = reinterpret_cast<uint32_t*>(buffer);
        float* posPtr = reinterpret_cast<float*>(buffer + 4);
        
        *idPtr = clientId;
        posPtr[0] = transform.position.x;
        posPtr[1] = transform.position.y;
        posPtr[2] = transform.position.z;
        
        msg.SetData(buffer, 256);
        
        networkManager_->Broadcast(msg);
    }
}

void DemoServer::ProcessClientInput(uint32_t clientId, const InputComponent& input) {
    auto it = clientEntities_.find(clientId);
    if (it == clientEntities_.end()) return;
    
    ecs::Entity entity = it->second;
    
    if (!world_.HasComponent<ecs::TransformComponent>(entity)) return;
    
    auto& transform = world_.GetComponent<ecs::TransformComponent>(entity);
    
    float moveSpeed = 10.0f;
    transform.position.x += input.moveAxis.x * moveSpeed * tickInterval_;
    transform.position.z += input.moveAxis.z * moveSpeed * tickInterval_;
    
    if (input.jump && transform.position.y == 0.0f) {
        transform.position.y = 2.0f;
    }
}

} // namespace demo
} // namespace ge